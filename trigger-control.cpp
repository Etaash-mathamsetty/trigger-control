#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <assert.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_sdlrenderer.h"
#include "icon.h"
#ifdef __linux__
#include <glib-2.0/glib.h>
#include <SDL2/SDL_mixer.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#include <shlobj.h>
#include "imgui/imgui_impl_win32.h"
#endif
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <locale>
#include <codecvt>
#include <chrono>
#include <thread>

#if !SDL_VERSION_ATLEAST(2, 0, 14)
#error SDL2 version 2.0.14 or higher is required
#endif

const char *VERSION = "Version 1.4";
char *CONFIG_PATH = new char[PATH_MAX];

//const uint8_t seed = 0xA2;
enum class dualsense_modes
{
	Off = 0x0,	 // no resistance
	Rigid = 0x1, // continous resistance
	Pulse = 0x2, // section resistance
	Rigid_A = 0x1 | 0x20,
	Rigid_B = 0x1 | 0x04,
	Rigid_AB = 0x1 | 0x20 | 0x04,
	Pulse_A = 0x2 | 0x20,
	Pulse_B = 0x2 | 0x04,
	Pulse_AB = 0x2 | 0x20 | 0x04,
};

void create_config_path_dir()
{
#ifdef __linux__
	if (!std::filesystem::is_directory(CONFIG_PATH))
		std::filesystem::create_directory(CONFIG_PATH);
#endif
#ifdef _WIN32
	// lmao windows with the wide char paths
	std::wstring wc(strlen(CONFIG_PATH), L'#');
	mbstowcs(&wc[0], CONFIG_PATH, strlen(CONFIG_PATH));
	if (!std::filesystem::is_directory(wc))
		std::filesystem::create_directory(wc);
#endif
}

void load_preset(uint8_t *outReport, const char *name)
{
	create_config_path_dir();
	std::string path = std::string(CONFIG_PATH);
	path += name;
	path += ".txt";
	// std::cout << path << std::endl;
	FILE *f = fopen(path.c_str(), "rb");
	if (f)
	{
		fread(outReport + 11, sizeof(*outReport), 30 - 10, f);
		fclose(f);
	}
	// printf("stub!\n");
}

void save_preset(const uint8_t *outReport, const char *name)
{
	create_config_path_dir();
	// printf("stub!\n");
	std::string path = std::string(CONFIG_PATH) + name + ".txt";
	// open(path.c_str(), O_RDWR | O_CREAT, 0777);
	FILE *f = fopen(path.c_str(), "wb");
	if (!f)
		return;
	fseek(f, 0, SEEK_SET);
	fwrite(outReport + 11, sizeof(*outReport), 30 - 10, f);
	fclose(f);
}

void write_config(char *value, size_t size)
{
	create_config_path_dir();
	std::string path = std::string(CONFIG_PATH) + "config.ini";
	FILE *f = fopen(path.c_str(), "wb");
	if (!f)
		return;
	fseek(f, 0, SEEK_SET);
	fwrite(value, sizeof(value), size, f);
	fclose(f);
}

void read_config(char **value, size_t size)
{
	create_config_path_dir();
	std::string path = std::string(CONFIG_PATH) + "config.ini";
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		return;
	fseek(f, 0, SEEK_SET);
	fread(*value, sizeof(**value), size, f);
	fclose(f);
}

void CenteredText(const char *text)
{
	ImVec2 size = ImGui::GetWindowSize();
	ImGui::SetCursorPosX((size.x - ImGui::CalcTextSize(text).x) / 2);
	//fine compiler you win...
	ImGui::Text("%s", text);
}

void error_sound()
{
#ifdef __linux__
	g_autofree gchar *name = g_build_filename(g_get_user_data_dir(), "sounds", "__custom", NULL);
	g_autofree gchar *path = g_build_filename(name, "bell-terminal.ogg", NULL);
	Mix_Chunk *sound = Mix_LoadWAV((const char *)path);
	Mix_PlayChannel(-1, sound, 0);
#endif
#ifdef _WIN32
	MessageBeep(MB_ICONERROR);
#endif
}

// I spent so long realizing that it was copying the pointer instead of modifying the pointer's address :/
int find_dev(SDL_GameController **handle)
{
	if (SDL_NumJoysticks() < 1)
		return -1;

	for (int i = 0; i < SDL_NumJoysticks(); i++)
	{
		if (SDL_IsGameController(i))
		{
			*handle = SDL_GameControllerOpen(i);
			if (*handle)
			{
				if (SDL_GameControllerGetType(*handle) == SDL_CONTROLLER_TYPE_PS5)
				{
					return 0;
				}
			}
			else
			{
				SDL_GameControllerClose(*handle);
			}
		}
	}
	// printf("%ls\n", hid_error(*handle));
	return -1;
}

dualsense_modes get_mode(int index)
{
	switch (index)
	{
	case 0:
		return dualsense_modes::Off;
	case 1:
		return dualsense_modes::Rigid;
	case 2:
		return dualsense_modes::Pulse;
	case 3:
		return dualsense_modes::Rigid_A;
	case 4:
		return dualsense_modes::Rigid_B;
	case 5:
		return dualsense_modes::Rigid_AB;
	case 6:
		return dualsense_modes::Pulse_A;
	case 7:
		return dualsense_modes::Pulse_B;
	case 8:
		return dualsense_modes::Pulse_AB;
	default:
		break;
	}
	return dualsense_modes::Off;
}

int get_index(dualsense_modes mode)
{
	switch (mode)
	{
	case dualsense_modes::Off:
		return 0;
	case dualsense_modes::Rigid:
		return 1;
	case dualsense_modes::Pulse:
		return 2;
	case dualsense_modes::Rigid_A:
		return 3;
	case dualsense_modes::Rigid_B:
		return 4;
	case dualsense_modes::Rigid_AB:
		return 5;
	case dualsense_modes::Pulse_A:
		return 6;
	case dualsense_modes::Pulse_B:
		return 7;
	case dualsense_modes::Pulse_AB:
		return 8;
	default:
		break;
	}
	return 0;
}

// this code is satisfying to look at
bool VectorOfStringGetter(void *data, int n, const char **out_text)
{
	const std::vector<std::string> &v = *(std::vector<std::string> *)data;
	*out_text = v[n].c_str();
	return true;
}

void apply_effect(SDL_GameController *dev, uint8_t *outReport)
{
	if (!dev)
		return;
	outReport[0] = 0x2;
	outReport[1] = 0x04 | 0x08;
	outReport[2] = 0x40;
	SDL_GameControllerSendEffect(dev, outReport + 1, 65);
}

void get_presets(std::vector<std::string> &options)
{
#ifdef __linux__
	for (const auto &file : std::filesystem::directory_iterator(CONFIG_PATH))
	{
		std::string filename = file.path().filename();
		filename = filename.substr(0, filename.find_last_of("."));
		if (file.path().extension() == ".txt")
		{
			options.push_back(filename);
		}
		// options.push_back(filename);
	}
#endif
#ifdef _WIN32
	for (const auto &file : std::filesystem::directory_iterator(CONFIG_PATH))
	{
		std::wstring str = file.path().filename();
		std::string str2(str.begin(), str.end());
		str2 = str2.substr(0, str2.find_last_of("."));
		if (file.path().extension() == L".txt")
		{
			options.push_back(str2);
		}
		//	options.push_back(str2);
	}
#endif
}

int main(int argc, char **argv)
{
	memset(CONFIG_PATH, 0, PATH_MAX);
#ifdef __linux__
	strcpy(CONFIG_PATH, getenv("HOME"));
	strcat(CONFIG_PATH, "/.config/trigger-control/");
#endif
#ifdef _WIN32
	strcpy(CONFIG_PATH, getenv("APPDATA"));
	strcat(CONFIG_PATH, "\\trigger-control\\");
#endif
	const size_t config_size = 10;
	char *config = (char *)alloca(sizeof(char) * config_size);
	memset(config, 0, config_size);
	// printf("sizeof bool: %d\n", sizeof(bool));
#ifdef __linux__
#if SDL_VERSION_ATLEAST(2, 0, 22)
	SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_PREFER_LIBDECOR, "1");
	SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
	printf("running in wayland mode!\n");
#endif
#endif
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO);
	uint32_t WindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	SDL_Window *window = SDL_CreateWindow("Trigger Controls", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 700, 620, WindowFlags);
	SDL_SetWindowMinimumSize(window, 500, 400);
	int height = 0, width = 0;
	SDL_GetWindowSize(window, &width, &height);
	assert(window);
	//SDL_GLContext context = SDL_GL_CreateContext(window);
	//SDL_GL_MakeCurrent(window, context);
	SDL_Surface *surface;
	surface = SDL_CreateRGBSurfaceWithFormatFrom(gimp_image.pixel_data, gimp_image.width, gimp_image.height, gimp_image.bytes_per_pixel * 8, 4 * gimp_image.width, SDL_PIXELFORMAT_RGBA32);
	SDL_SetWindowIcon(window, surface);
	SDL_FreeSurface(surface);
#ifdef __linux__
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		// error_sound(); lmao cannot play the sound without mixer
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "could not initialize sdl_mixer", window);
		exit(EXIT_FAILURE);
	}
#endif
	//glewExperimental = true;
	//glewInit();
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	bool popup_open = false;
	bool save_preset_open = false;
	bool load_preset_open = false;
	bool delete_preset_open = false;
	bool preset_exists = false;
	bool options_open = false;
	bool controller_navigation_help_open = false;
	char name[100];
	// name.reserve(100);
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	read_config(&config, config_size);
	if (config[0])
		ImGui::StyleColorsDark();
	else
		ImGui::StyleColorsLight();

	ImGui::GetStyle().WindowRounding = 5.0f;
	ImGui::GetStyle().PopupRounding = 5.0f;
	ImGui::GetStyle().FrameRounding = 5.0f;
	ImGui::GetStyle().GrabRounding = 5.0f;
	// setup platform/renderer bindings
	//ImGui_ImplSDL2_InitForOpenGL(window, context);
	//ImGui_ImplOpenGL3_Init("#version 100");
	//SDL_GL_SetSwapInterval(1);
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer_Init(renderer);
// float dpi_scaling = 1.0f;
#ifdef _WIN32
	float dpi_x, dpi_y, dpi_z;
	SDL_GetDisplayDPI(0, &dpi_x, &dpi_y, &dpi_z);
	float dpi_scaling = dpi_x / 96.0f;
	// std::cout << dpi_scaling << std::endl;
	// SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"DPI",std::to_string(dpi_scaling).c_str(),window);
	ImGui::GetStyle().ScaleAllSizes(dpi_scaling);
	std::string windir = getenv("WINDIR");
	if (std::filesystem::exists(windir + "\\Fonts\\segoeui.ttf"))
	{
		io.Fonts->AddFontFromFileTTF((windir + "\\Fonts\\segoeui.ttf").c_str(), 36.0f * dpi_scaling);
		io.FontGlobalScale = 0.5f;
	}
#endif
#ifdef __linux__
	// should work for some people, but not all
	// probably arch based distros
		ImVector<ImWchar> ranges;
		ImFontGlyphRangesBuilder builder;
		builder.AddChar(L'Δ'); 
		builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
		builder.BuildRanges(&ranges);
	if (std::filesystem::exists("/usr/share/fonts/TTF/DejaVuSans.ttf"))
	{
		io.Fonts->AddFontFromFileTTF("/usr/share/fonts/TTF/DejaVuSans.ttf", 18.0f, NULL, ranges.Data);
	}
	// probably ubuntu based
	else if (std::filesystem::exists("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
	{
		io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 18.0f, NULL, ranges.Data);
	}
	else
	{
		printf("could not find font\n");
	}
#endif
	// bool bt = false;
	//  char* path = NULL;
	int preset_index = 0;
	SDL_GameController *handle;
	int res = find_dev(&handle);
	if (res == -1)
	{
		error_sound();
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "could not find a dualsense controller!", window);
		std::cout << "error: " << SDL_GetError() << std::endl;
		exit(EXIT_FAILURE);
	}
	// printf("%d\n",bt);
	// free(path);
	bool running = true;
	// SDL_SetWindowResizable(window, SDL_bool::SDL_TRUE);
	uint8_t *outReport = new uint8_t[78];
	memset(outReport, 0, 78);
	outReport[0] = 0x2;
	outReport[1] = 0x04 | 0x08;
	outReport[2] = 0x40;
	const char *states[9] = {"Off", "Rigid", "Pulse", "RigidA", "RigidB", "RigidAB", "PulseA", "PulseB", "PulseAB"};
	int left_cur = 0;
	int right_cur = 0;
	float light_colors[3] = {0};
	light_colors[2] = 70 / 255.0;
	int player = 1;
	int cur_tab = 0;
	while (running)
	{

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
			{
				running = false;
			}
			if (event.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				SDL_GetWindowSize(window, &width, &height);
				//glViewport(0, 0, width, height);
			}
		}
#define APPLY()                                                 \
	outReport[11] = (uint8_t)dualsense_modes::Rigid_B;          \
	outReport[22] = (uint8_t)dualsense_modes::Rigid_B;          \
	apply_effect(handle, outReport);                            \
	std::this_thread::sleep_for(std::chrono::milliseconds(70)); \
	outReport[11] = (uint8_t)get_mode(right_cur);               \
	outReport[22] = (uint8_t)get_mode(left_cur);                \
	apply_effect(handle, outReport)

		// const wchar_t *error = hid_error(handle);
		if (SDL_GameControllerGetAttached(handle) == SDL_FALSE)
		{
#ifdef __linux__
			sleep(1);
#endif
#ifdef _WIN32
			Sleep(1000);
#endif
			SDL_GameControllerClose(handle);
			int res = find_dev(&handle);
			if (res == -1)
			{
				error_sound();
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "Controller Disconnected! (or something else happened)", window);
				std::cout << "error: " << SDL_GetError() << std::endl;
				exit(EXIT_FAILURE);
			}
			APPLY();
		}
		//glClearColor(0.f, 0.f, 0.f, 0.f);
		//glClear(GL_COLOR_BUFFER_BIT);
		//ImGui_ImplOpenGL3_NewFrame();
		SDL_SetRenderDrawColor(renderer,0,0,0,0);
        SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		ImGui::SetNextWindowSize(
			ImVec2(float(width), float(height)),
			ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
		// ImGui::ShowDemoWindow();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::Begin("Controls", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar();

		//	ImGui::ShowDemoWindow();
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Load Preset"))
				{
					load_preset_open = true;
					memset(name, 0, sizeof(name));
					// name.clear();
				}
				if (ImGui::MenuItem("Save Preset"))
				{
					save_preset_open = true;
					memset(name, 0, sizeof(name));
					// name.clear();
				}
				if (ImGui::MenuItem("Delete Preset"))
				{
					delete_preset_open = true;
					memset(name, 0, sizeof(name));
					// name.clear();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Tools"))
			{
				if (ImGui::MenuItem("Options"))
				{
					options_open = true;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("Controller Navigation"))
				{
					controller_navigation_help_open = true;
				}
				if (ImGui::MenuItem("About"))
				{
					popup_open = true;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		if (controller_navigation_help_open)
		{
			ImGui::OpenPopup("Controller Navigation Help");
		}
		if (popup_open)
		{
			ImGui::OpenPopup("About");
		}
		if (load_preset_open)
		{
			ImGui::OpenPopup("Load Preset");
		}
		if (save_preset_open)
		{
			ImGui::OpenPopup("Save Preset");
		}
		if (delete_preset_open)
		{
			ImGui::OpenPopup("Delete Preset");
		}
		if (options_open)
		{
			ImGui::OpenPopup("Options");
		}
		const int num_tabs = 2;
		bool left_shoulder = SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
		bool right_shoulder = SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
		if (left_shoulder && cur_tab > 0)
		{
			cur_tab--;
		}
		if (right_shoulder && cur_tab < num_tabs - 1)
		{
			cur_tab++;
		}
		if (ImGui::BeginTabBar("tabs"))
		{
			ImGuiTabItemFlags flags[num_tabs] = {ImGuiTabBarFlags_None};
			if (left_shoulder || right_shoulder)
			{
				flags[cur_tab] |= ImGuiTabItemFlags_SetSelected;
			}
			if (ImGui::BeginTabItem("Trigger Control", nullptr, flags[0]))
			{
				if (ImGui::Button("Reset"))
				{
					memset(outReport, 0, 78);
					outReport[11] = (uint8_t)dualsense_modes::Rigid_B;
					outReport[22] = (uint8_t)dualsense_modes::Rigid_B;
					apply_effect(handle, outReport);
					left_cur = 0;
					right_cur = 0;
					// printf("reset!\n");
					outReport[11] = (uint8_t)0;
					outReport[22] = (uint8_t)0;
				}

				ImGui::Text("Right Trigger:");
				ImGui::Combo("Right Mode", &right_cur, states, IM_ARRAYSIZE(states));
				uint8_t min = 0;
				uint8_t max = UINT8_MAX;
				outReport[11] = static_cast<uint8_t>(get_mode(right_cur));
#define SLIDER(str, ptr) ImGui::SliderScalar(str, ImGuiDataType_U8, ptr, &min, &max, "%d")
				SLIDER("Right Start Intensity", &outReport[12]);
				SLIDER("Right Effect Force", &outReport[13]);
				SLIDER("Right Range Force", &outReport[14]);
				SLIDER("Right Near Release Strength", &outReport[15]);
				SLIDER("Right Near Middle Strength", &outReport[16]);
				SLIDER("Right Pressed Strength", &outReport[17]);
				SLIDER("Right Actuation Frequency", &outReport[20]);
				ImGui::Text("Left Trigger:");
				ImGui::Combo("Left Mode", &left_cur, states, IM_ARRAYSIZE(states));
				outReport[22] = static_cast<uint8_t>(get_mode(left_cur));
				SLIDER("Left Start Resistance", &outReport[23]);
				SLIDER("Left Effect Force", &outReport[24]);
				SLIDER("Left Range Force", &outReport[25]);
				SLIDER("Left Near Release Strength", &outReport[26]);
				SLIDER("Left Near Middle Strength", &outReport[27]);
				SLIDER("Left Pressed Strength", &outReport[28]);
				SLIDER("Left Actuation Frequency", &outReport[30]);
				if (ImGui::Button("Apply"))
				{
					// printf("applied! bt: %d\n", bt);
					APPLY();
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Light Control", nullptr, flags[1]))
			{
				ImGui::ColorPicker3("Light Color", light_colors);
				ImGui::SliderInt("Player Number", &player, 1, 4);
				if (ImGui::Button("Apply"))
				{
					SDL_GameControllerSetLED(handle, light_colors[0] * UINT8_MAX, light_colors[1] * UINT8_MAX, light_colors[2] * UINT8_MAX);
					SDL_GameControllerSetPlayerIndex(handle, player - 1);
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		if (ImGui::BeginPopupModal("Controller Navigation Help", &controller_navigation_help_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::SetWindowSize(ImVec2(510, 170), ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth() / 2;
			_pos.y -= ImGui::GetWindowHeight() / 2;
			ImGui::SetWindowPos(_pos);
			ImGui::BulletText("Press the left shoulder button to go back a tab.");
			ImGui::BulletText("Press the right shoulder button to go forward a tab.");
			ImGui::BulletText("Press O to close popups");
			ImGui::BulletText("Press Δ to perform the action in the current popup");
			ImGui::BulletText("Press X to change the value of something (like a slider)");
			ImGui::BulletText("Use the d-pad to navigate the menus and change the sliders");
			if (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_B))
			{
				controller_navigation_help_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupModal("About", &popup_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			// printf("about2!\n");
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth() / 2;
			_pos.y -= ImGui::GetWindowHeight() / 2;
			ImGui::SetWindowPos(_pos);
			CenteredText("Trigger Control");
			CenteredText(VERSION);
			ImGui::Separator();
			ImGui::Text("Made with FOSS, Powered by ImGui");
			if (SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_B))
			{
				popup_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupModal("Load Preset", &load_preset_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::SetWindowSize(ImVec2(300, 100), ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth() / 2;
			_pos.y -= ImGui::GetWindowHeight() / 2;
			ImGui::SetWindowPos(_pos);
			std::vector<std::string> options;
			get_presets(options);
			ImGui::Combo("Presets", &preset_index, VectorOfStringGetter, &options, options.size());
			if ((ImGui::Button("Load") || SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_Y)) && options.size() > 0)
			{
				load_preset(outReport, options[preset_index].c_str());
				right_cur = get_index(static_cast<dualsense_modes>(outReport[11]));
				left_cur = get_index(static_cast<dualsense_modes>(outReport[22]));
				APPLY();
				load_preset_open = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel") || SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_B))
			{
				load_preset_open = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		if (preset_exists)
		{
			ImGui::OpenPopup("Preset Exists");
		}
		if (ImGui::BeginPopup("Preset Exists", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::SetWindowSize(ImVec2(300, 180), ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth() / 2;
			_pos.y -= ImGui::GetWindowHeight() / 2;
			ImGui::SetWindowPos(_pos);
			ImGui::Text("This Preset Already Exists! Are You Sure You Want To Overwrite It?");
			if (ImGui::Button("Yes"))
			{
				save_preset(outReport, name);
				save_preset_open = false;
				ImGui::CloseCurrentPopup();
				preset_exists = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("No"))
			{
				ImGui::CloseCurrentPopup();
				preset_exists = false;
				save_preset_open = true;
			}

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Save Preset", &save_preset_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::SetWindowSize(ImVec2(340, 100), ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth() / 2;
			_pos.y -= ImGui::GetWindowHeight() / 2;
			ImGui::SetWindowPos(_pos);
			std::vector<std::string> options;
			get_presets(options);
			if (ImGui::InputTextWithHint("Preset Name", "Name", name, IM_ARRAYSIZE(name), ImGuiInputTextFlags_EnterReturnsTrue) && name[0] != '\0')
			{
				auto is_name = [name](std::string a)
				{
					return strcmp(a.c_str(), name) == 0;
				};
				if (std::find_if(options.begin(), options.end(), is_name) != options.end())
				{
					printf("Preset already exists!\n");
					// ImGui::CloseCurrentPopup();
					save_preset_open = false;
					// ImGui::OpenPopup("Preset Exists!");
					preset_exists = true;
				}
				else
				{
					save_preset(outReport, name);
					save_preset_open = false;
				}
			}

			if ((ImGui::Button("Save")) && name[0] != '\0')
			{
				auto is_name = [name](std::string a)
				{
					return strcmp(a.c_str(), name) == 0;
				};
				if (std::find_if(options.begin(), options.end(), is_name) != options.end())
				{
					printf("Preset already exists!\n");
					// preset_exists = true;
					// ImGui::CloseCurrentPopup();
					save_preset_open = false;
					// ImGui::OpenPopup("Preset Exists!");
					preset_exists = true;
				}
				else
				{
					save_preset(outReport, name);
					save_preset_open = false;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel") || SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_B))
			{
				save_preset_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Delete Preset", &delete_preset_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::SetWindowSize(ImVec2(300, 120), ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth() / 2;
			_pos.y -= ImGui::GetWindowHeight() / 2;
			ImGui::SetWindowPos(_pos);
			std::vector<std::string> options;
			get_presets(options);
			ImGui::Text("You cannot undo this action!");
			ImGui::Combo("Presets", &preset_index, VectorOfStringGetter, &options, options.size());
			if ((ImGui::Button("Delete!") || SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_Y)) && options.size() > 0)
			{
				remove((std::string(CONFIG_PATH) + options[preset_index] + ".txt").c_str());
				delete_preset_open = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel") || SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_B))
			{
				delete_preset_open = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Options", &options_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::SetWindowSize(ImVec2(300, 120), ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth() / 2;
			_pos.y -= ImGui::GetWindowHeight() / 2;
			ImGui::SetWindowPos(_pos);
			// ImGui::Text("It's Empty here :(");

			ImGui::Checkbox("Dark Mode", (bool *)&config[0]);
			if (config[0])
			{
				ImGui::StyleColorsDark();
			}
			else
			{
				ImGui::StyleColorsLight();
			}
			ImGui::Checkbox("Reset On Close", (bool *)&config[1]);
			if (ImGui::Button("Close") || SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_B))
			{
				options_open = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
		ImGui::End();
		ImGui::Render();
		ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
		SDL_RenderPresent(renderer);
		
	}
	ImGui_ImplSDLRenderer_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	//SDL_GL_DeleteContext(context);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

#ifdef __linux__
	Mix_CloseAudio();
	Mix_Quit();
#endif
	// hid_close(handle);
	// hid_exit();
	if (config[1])
	{
		memset(outReport, 0, 78);
		outReport[11] = (uint8_t)dualsense_modes::Rigid_B;
		outReport[22] = (uint8_t)dualsense_modes::Rigid_B;
		apply_effect(handle, outReport);
		left_cur = 0;
		right_cur = 0;
		// printf("reset!\n");
		outReport[11] = (uint8_t)0;
		outReport[22] = (uint8_t)0;
	}
	SDL_GameControllerClose(handle);
	delete[] outReport;
	SDL_Quit();
	if (std::filesystem::exists("imgui.ini"))
		std::filesystem::remove("imgui.ini");
	write_config(config, config_size);
	// program termination should free memory I forgot to free :D
	return 0;
}
