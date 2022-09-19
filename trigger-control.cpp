#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <assert.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_sdlrenderer.h"
#include "imgui/imfilebrowser.h"
#include "icon.h"
#ifdef __linux__
#include <linux/limits.h>
#include <dbus/dbus.h>
#include <optional>
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
#include "libtrigger-control/ds-trigger-control.h"
using namespace triggercontrol;

#if !SDL_VERSION_ATLEAST(2, 0, 14)
#error SDL2 version 2.0.14 or higher is required
#endif

const char *VERSION = "Version 1.5";
char *CONFIG_PATH = new char[PATH_MAX];

void create_config_path_dir()
{
#ifdef __linux__
	if (!std::filesystem::is_directory(CONFIG_PATH))
		std::filesystem::create_directory(CONFIG_PATH);
#endif
#ifdef _WIN32
	// lmao windows with the wide char paths
	std::wstring wc(std::string(CONFIG_PATH).begin(), std::string(CONFIG_PATH).end());
	if (!std::filesystem::is_directory(wc))
		std::filesystem::create_directory(wc);
#endif
}

void load_preset(uint8_t *rightEffects, uint8_t *leftEffects, const char *name)
{
	create_config_path_dir();
	std::string path = std::string(CONFIG_PATH);
	path += name;
	path += ".txt";
	FILE *f = fopen(path.c_str(), "rb");
	if (f)
	{
		fread(rightEffects, sizeof(*rightEffects), 7, f);
		fread(leftEffects, sizeof(*leftEffects), 7, f);
		fclose(f);
	}
}

void save_preset(const uint8_t *rightEffects, const uint8_t *leftEffects, const char *name)
{
	create_config_path_dir();
	std::string path = std::string(CONFIG_PATH) + name + ".txt";
	FILE *f = fopen(path.c_str(), "wb");
	if (!f)
		return;
	fseek(f, 0, SEEK_SET);
	fwrite(rightEffects, sizeof(*rightEffects), 7, f);
	fwrite(leftEffects, sizeof(*leftEffects), 7, f);
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

inline void CenteredText(const char *text)
{
	ImVec2 size = ImGui::GetWindowSize();
	ImGui::SetCursorPosX((size.x - ImGui::CalcTextSize(text).x) / 2);
	// fine compiler you win...
	ImGui::Text("%s", text);
}

inline void error_sound()
{
#ifdef __linux__
	// unfortunately this is not going to work on anything other than gnome
	/*g_autofree gchar *name = g_build_filename(g_get_user_data_dir(), "sounds", "__custom", NULL);
	g_autofree gchar *path = g_build_filename(name, "bell-terminal.ogg", NULL);
	Mix_Chunk *sound = Mix_LoadWAV((const char *)path);
	Mix_PlayChannel(-1, sound, 0);*/
	std::cout << '\a' << std::endl;
#endif
#ifdef _WIN32
	MessageBeep(MB_ICONERROR);
#endif
}

inline ds::modes get_mode(int index)
{
	switch (index)
	{
	case 0:
		return ds::modes::Off;
	case 1:
		return ds::modes::Rigid;
	case 2:
		return ds::modes::Pulse;
	case 3:
		return ds::modes::Rigid_A;
	case 4:
		return ds::modes::Rigid_B;
	case 5:
		return ds::modes::Rigid_AB;
	case 6:
		return ds::modes::Pulse_A;
	case 7:
		return ds::modes::Pulse_B;
	case 8:
		return ds::modes::Pulse_AB;
	default:
		break;
	}
	return ds::modes::INVALID;
}

inline int get_index(ds::modes mode)
{
	switch (mode)
	{
	case ds::modes::Off:
		return 0;
	case ds::modes::Rigid:
		return 1;
	case ds::modes::Pulse:
		return 2;
	case ds::modes::Rigid_A:
		return 3;
	case ds::modes::Rigid_B:
		return 4;
	case ds::modes::Rigid_AB:
		return 5;
	case ds::modes::Pulse_A:
		return 6;
	case ds::modes::Pulse_B:
		return 7;
	case ds::modes::Pulse_AB:
		return 8;
	default:
		break;
	}
	return -1;
}

// this code is satisfying to look at
bool VectorOfStringGetter(void *data, int n, const char **out_text)
{
	const std::vector<std::string> &v = *(std::vector<std::string> *)data;
	*out_text = v[n].c_str();
	return true;
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
	}
#endif
}

bool check_valid(std::string path)
{
	// std::cout << std::filesystem::file_size(path) << std::endl;
	if (std::filesystem::file_size(path) != 20)
	{
		return false;
	}
	FILE *f = fopen(path.c_str(), "rb");
	uint8_t x = fgetc(f);
	fclose(f);
	const uint8_t valid_modes[9] = {0x0, 0x1, 0x2, 0x1 | 0x20, 0x1 | 0x04, 0x1 | 0x20 | 0x04, 0x2 | 0x20, 0x2 | 0x04, 0x2 | 0x20 | 0x04};
	if (std::find(std::begin(valid_modes), std::end(valid_modes), x) == std::end(valid_modes))
	{
		return false;
	}
	return true;
}

inline void center_window()
{
	ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
	_pos.x -= ImGui::GetWindowWidth() / 2;
	_pos.y -= ImGui::GetWindowHeight() / 2;
	ImGui::SetWindowPos(_pos);
}

#ifdef __linux__

template <typename T>
std::optional<T> get_dbus_property(DBusConnection *con, const char *path, const char *property, const char *interface)
{
	DBusMessage *msg = dbus_message_new_method_call("org.bluez", path, "org.freedesktop.DBus.Properties", "Get");
	DBusMessageIter args;
	dbus_message_iter_init_append(msg, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &interface);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &property);
	DBusError err;
	dbus_error_init(&err);
	DBusMessage *reply = dbus_connection_send_with_reply_and_block(con, msg, -1, &err);
	if (dbus_error_is_set(&err))
	{
		std::cerr << "Failed to get dbus property! " << err.message << std::endl;
		return {};
	}
	dbus_message_unref(msg);
	DBusMessageIter iter;
	DBusMessageIter va_iter;
	dbus_message_iter_init(reply, &iter);
	dbus_message_iter_recurse(&iter, &va_iter);
	T value;
	dbus_message_iter_get_basic(&va_iter, &value);
	dbus_message_unref(reply);
	return value;
}

std::optional<std::string> find_dualsense(SDL_GameController *controller)
{
	DBusError err;
	dbus_error_init(&err);
	DBusConnection *con = NULL;
	con = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (dbus_error_is_set(&err))
	{
		std::cerr << "failed to connect to dbus system bus " << err.message << std::endl;
		return {};
	}
	DBusMessage *msg = dbus_message_new_method_call("org.bluez", "/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
	DBusMessage *reply = dbus_connection_send_with_reply_and_block(con, msg, -1, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err))
	{
		std::cerr << "failed to get managed objects " << err.message << std::endl;
		return {};
	}
	DBusMessageIter dict;
	dbus_message_iter_init(reply, &dict);
	DBusMessageIter dict_entry;
	dbus_message_iter_recurse(&dict, &dict_entry);
	DBusMessageIter dict_kv;
	while (dbus_message_iter_get_arg_type(&dict_entry) != DBUS_TYPE_INVALID)
	{
		dbus_message_iter_recurse(&dict_entry, &dict_kv);
		char *path;
		dbus_message_iter_get_basic(&dict_kv, &path);
		dbus_message_iter_next(&dict_kv);
		DBusMessageIter iface_entry;
		dbus_message_iter_recurse(&dict_kv, &iface_entry);
		DBusMessageIter iface_kv;
		while (dbus_message_iter_get_arg_type(&iface_entry) != DBUS_TYPE_INVALID)
		{
			dbus_message_iter_recurse(&iface_entry, &iface_kv);
			char *value;
			dbus_message_iter_get_basic(&iface_kv, &value);
			if (!strcmp(value, "org.bluez.Device1"))
			{
				std::string serial = SDL_GameControllerGetSerial(controller);
				for (char &c : serial)
				{
					c = toupper(c);
					if (c == '-')
						c = ':';
				}
				auto connected = get_dbus_property<bool>(con, path, "Connected", "org.bluez.Device1");
				auto value = get_dbus_property<char *>(con, path, "Address", "org.bluez.Device1");
				if (value.has_value() && connected.has_value() && *connected && serial == *value)
				{
					// std::cout << "found dualsense controller at: " << path << std::endl;
					return std::string(path);
				}
			}
			dbus_message_iter_next(&iface_entry);
		}
		dbus_message_iter_next(&dict_entry);
	}
	dbus_message_unref(reply);
	dbus_connection_unref(con);
	return {};
}

bool disconnect_dualsense(std::string path)
{
	DBusError err;
	dbus_error_init(&err);
	DBusConnection *con = NULL;
	con = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	DBusMessage *msg = dbus_message_new_method_call("org.bluez", path.c_str(), "org.bluez.Device1", "Disconnect");
	DBusMessage *reply = dbus_connection_send_with_reply_and_block(con, msg, -1, &err);
	dbus_message_unref(msg);
	if (dbus_error_is_set(&err))
	{
		std::cerr << "failed to disconnect device: " << err.message << std::endl;
		return false;
	}
	dbus_message_unref(reply);
	dbus_connection_unref(con);
	return true;
}

#endif

int main(int argc, char **argv)
{
	SDL_version sdl_version;
	SDL_GetVersion(&sdl_version);
	if (sdl_version.minor == 0 && sdl_version.patch < 14)
	{
		std::cerr << "YOUR SDL VERSION IS TOO OLD!" << std::endl;
	}
	memset(CONFIG_PATH, 0, PATH_MAX);
#ifdef __linux__
	const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
	if (xdg_config_home && std::filesystem::exists(xdg_config_home) && std::filesystem::is_directory(xdg_config_home))
		strcpy(CONFIG_PATH, xdg_config_home);
	else
	{
		strcpy(CONFIG_PATH, getenv("HOME"));
		strcat(CONFIG_PATH, "/.config");
	}
	strcat(CONFIG_PATH, "/trigger-control/");
#endif
#ifdef _WIN32
	strcpy(CONFIG_PATH, getenv("APPDATA"));
	strcat(CONFIG_PATH, "\\trigger-control\\");
	ImGui_ImplWin32_EnableDpiAwareness();
#endif
	const size_t config_size = 10;
	char *config = (char *)alloca(sizeof(char) * config_size);
	memset(config, 0, config_size);
#ifdef __linux__
#if SDL_VERSION_ATLEAST(2, 0, 22)
	if (sdl_version.minor > 0 || sdl_version.patch >= 22)
	{
		SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_PREFER_LIBDECOR, "1");
		SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
	}
#endif
#endif
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_PLAYER_LED, "1");
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
	uint32_t WindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	SDL_Window *window = SDL_CreateWindow("Trigger Controls", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 700, 620, WindowFlags);
	std::cout << "running using " << SDL_GetCurrentVideoDriver() << " video driver" << std::endl;
	SDL_SetWindowMinimumSize(window, 500, 400);
	int height = 0, width = 0;
	SDL_GetWindowSize(window, &width, &height);
	assert(window);
	SDL_Surface *surface;
	surface = SDL_CreateRGBSurfaceWithFormatFrom(gimp_image.pixel_data, gimp_image.width, gimp_image.height, gimp_image.bytes_per_pixel * 8, 4 * gimp_image.width, SDL_PIXELFORMAT_RGBA32);
	SDL_SetWindowIcon(window, surface);
	SDL_FreeSurface(surface);
	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	bool popup_open = false;
	bool save_preset_open = false;
	bool load_preset_open = false;
	bool delete_preset_open = false;
	bool preset_exists = false;
	bool options_open = false;
	bool controller_navigation_help_open = false;
	bool export_preset = false;
	char name[100];
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
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer_Init(renderer);
	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder builder;
	builder.AddChar(L'Δ');
	builder.AddChar(L'▢');
	builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
	builder.BuildRanges(&ranges);
	// wierd stuff for hidpi (it actually works though)
	int _width, _height;
	SDL_GetRendererOutputSize(renderer, &_width, &_height);
	float dpi_scaling = _width / (float)width;
	printf("dpi scaling: %f\n", dpi_scaling);
	ImGui::GetStyle().ScaleAllSizes(dpi_scaling);
	SDL_RenderSetScale(renderer, dpi_scaling, dpi_scaling);
	// float x,y,z;
	// SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(window),&x,&y,&z);
	// std::cout << "dpi: " << x << "," << y <<  "," << z << std::endl;
#ifdef _WIN32
	std::string windir = getenv("WINDIR");
	if (std::filesystem::exists(windir + "\\Fonts\\segoeui.ttf"))
	{
		io.Fonts->AddFontFromFileTTF((windir + "\\Fonts\\segoeui.ttf").c_str(), 18.0f * dpi_scaling, NULL, ranges.Data);
	}
#endif
#ifdef __linux__
	// probably should work for all distros
	for (auto file : std::filesystem::recursive_directory_iterator("/usr/share/fonts/"))
	{
		if (file.is_directory())
			continue;
		if (file.path().filename() == "DejaVuSans.ttf")
		{
			io.Fonts->AddFontFromFileTTF(file.path().c_str(), 18.0f * dpi_scaling, NULL, ranges.Data);
			break;
		}
	}
#endif
	int preset_index = 0;
	SDL_GameController *handle;
	int res = ds::find(&handle);
	if (res == -1)
	{
		error_sound();
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "could not find a dualsense controller!", window);
		std::cout << "error: " << SDL_GetError() << std::endl;
		exit(EXIT_FAILURE);
	}
#ifdef __linux__
	auto dbus_dualsense_path = find_dualsense(handle);
#endif
	bool running = true;
	uint8_t leftEffects[7] = {0};
	ds::modes leftMode = ds::modes::Off;
	uint8_t rightEffects[7] = {0};
	ds::modes rightMode = ds::modes::Off;
	const char *states[9] = {"Off", "Rigid", "Pulse", "RigidA", "RigidB", "RigidAB", "PulseA", "PulseB", "PulseAB"};
	int left_cur = 0;
	int right_cur = 0;
	float light_colors[3] = {0};
	light_colors[2] = 70 / 255.0;
	int player = 1;
	int cur_tab = 0;
	ImGui::FileBrowser fileDialog(ImGuiFileBrowserFlags_NoTitleBar);
	ImGui::FileBrowser fileDialog2(ImGuiFileBrowserFlags_NoTitleBar | ImGuiFileBrowserFlags_SelectDirectory);
	fileDialog.SetTitle("Choose Preset");
	fileDialog.SetTypeFilters({".txt"});
	fileDialog2.SetTitle("Where do you want to export the preset?");

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
			}
		}
#define APPLY()                                                          \
	ds::reset_all(handle);                                               \
	std::this_thread::sleep_for(std::chrono::milliseconds(70));          \
	ds::apply_effect(handle, ds::triggers::left, leftMode, leftEffects); \
	ds::apply_effect(handle, ds::triggers::right, rightMode, rightEffects)

		if (SDL_GameControllerGetAttached(handle) == SDL_FALSE)
		{
			std::this_thread::sleep_for(std::chrono::seconds(2));
			SDL_GameControllerClose(handle);
			int res = ds::find(&handle);
			if (res == -1)
			{
				error_sound();
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "Controller Disconnected! (or something else happened)", window);
				std::cout << "error: " << SDL_GetError() << std::endl;
				exit(EXIT_FAILURE);
			}
			APPLY();
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();
		ImGui::SetNextWindowSize(
			ImVec2(float(width), float(height)),
			ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::Begin("Controls", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar();

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Load Preset"))
				{
					load_preset_open = true;
					memset(name, 0, sizeof(name));
				}
				if (ImGui::MenuItem("Load Preset from File"))
				{
					fileDialog.Open();
					memset(name, 0, sizeof(name));
				}
				if (ImGui::MenuItem("Export Preset"))
				{
					fileDialog2.Open();
					memset(name, 0, sizeof(name));
				}
				if (ImGui::MenuItem("Save Preset"))
				{
					save_preset_open = true;
					memset(name, 0, sizeof(name));
				}
				if (ImGui::MenuItem("Delete Preset"))
				{
					delete_preset_open = true;
					memset(name, 0, sizeof(name));
				}
				if (ImGui::MenuItem("Exit"))
				{
					exit(EXIT_SUCCESS);
				}
#ifdef __linux__
				if (dbus_dualsense_path.has_value())
				{
					if (ImGui::MenuItem("Exit And Turn Off Controller"))
					{
						disconnect_dualsense(*dbus_dualsense_path);
						exit(EXIT_SUCCESS);
					}
				}
#endif
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
		ImGui::SetNextWindowFocus();
		// default is 700x450
		fileDialog.SetWindowPos((width / 2) - (700 / 2), (height / 2) - (450 / 2));
		fileDialog.Display();
		if (fileDialog.HasSelected())
		{
			std::string path = fileDialog.GetSelected().string();
			std::string __name;
#ifdef __linux__
			__name = path.substr(path.find_last_of("/") + 1);
#endif
#ifdef _WIN32
			__name = path.substr(path.find_last_of("\\") + 1);
#endif
			__name = __name.substr(0, __name.find_last_of("."));
			// copy preset into config path
			std::vector<std::string> options;
			get_presets(options);
			if (check_valid(path))
			{
				if (std::find(options.begin(), options.end(), __name) == options.end())
				{
					std::filesystem::copy(path, CONFIG_PATH);
					load_preset(rightEffects, leftEffects, __name.c_str());
					APPLY();
				}
				else
				{
					SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning", "Preset already exists", window);
				}
			}
			else
			{
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning", "Invalid Preset", window);
			}
			fileDialog.ClearSelected();
		}
		ImGui::SetNextWindowFocus();
		fileDialog2.SetWindowPos((width / 2) - (700 / 2), (height / 2) - (450 / 2));
		fileDialog2.Display();
		if (fileDialog2.HasSelected())
		{
			export_preset = true;
		}
		if (export_preset)
		{
			ImGui::OpenPopup("Export Preset");
		}
		if (ImGui::BeginPopup("Export Preset", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			center_window();
			std::vector<std::string> options;
			get_presets(options);
			ImGui::Combo("Presets", &preset_index, VectorOfStringGetter, &options, options.size());
			if (ImGui::Button("Export"))
			{
				std::string path = fileDialog2.GetSelected().string();
				if (std::filesystem::exists(path + options[preset_index] + ".txt"))
				{
					std::filesystem::remove(path + options[preset_index] + ".txt");
				}
				std::filesystem::copy(CONFIG_PATH + options[preset_index] + ".txt", path);
				export_preset = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
			fileDialog2.ClearSelected();
		}
		// load_preset(outReport,);
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
					memset(leftEffects, 0, 7);
					memset(rightEffects, 0, 7);
					ds::reset_all(handle);
					left_cur = 0;
					right_cur = 0;
					rightMode = ds::modes::Off;
					leftMode = ds::modes::Off;
				}
				const uint8_t min = 0;
				const uint8_t max = UINT8_MAX;
#define SLIDER(str, ptr) ImGui::SliderScalar(str, ImGuiDataType_U8, ptr, &min, &max, "%d")
				ImGui::Text("Right Trigger:");
				ImGui::Combo("Right Mode", &right_cur, states, IM_ARRAYSIZE(states));
				rightMode = get_mode(right_cur);
				SLIDER("Right Start Intensity", &rightEffects[0]);
				SLIDER("Right Effect Force", &rightEffects[1]);
				SLIDER("Right Range Force", &rightEffects[2]);
				SLIDER("Right Near Release Strength", &rightEffects[3]);
				SLIDER("Right Near Middle Strength", &rightEffects[4]);
				SLIDER("Right Pressed Strength", &rightEffects[5]);
				SLIDER("Right Actuation Frequency", &rightEffects[6]);
				ImGui::Text("Left Trigger:");
				ImGui::Combo("Left Mode", &left_cur, states, IM_ARRAYSIZE(states));
				leftMode = get_mode(left_cur);
				SLIDER("Left Start Resistance", &leftEffects[0]);
				SLIDER("Left Effect Force", &leftEffects[1]);
				SLIDER("Left Range Force", &leftEffects[2]);
				SLIDER("Left Near Release Strength", &leftEffects[3]);
				SLIDER("Left Near Middle Strength", &leftEffects[4]);
				SLIDER("Left Pressed Strength", &leftEffects[5]);
				SLIDER("Left Actuation Frequency", &leftEffects[6]);
				if (ImGui::Button("Apply"))
				{
					APPLY();
				}

				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Light Control", nullptr, flags[1]))
			{
				ImGui::ColorPicker3("Light Color", light_colors);
				if (sdl_version.minor >= 24)
				{
					ImGui::SliderInt("Player Number", &player, 0, 4);
				}
				else
				{
					ImGui::SliderInt("Player Number", &player, 1, 4);
				}
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
			center_window();
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
			center_window();
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
			center_window();
			std::vector<std::string> options;
			get_presets(options);
			ImGui::Combo("Presets", &preset_index, VectorOfStringGetter, &options, options.size());
			if ((ImGui::Button("Load") || SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_Y)) && options.size() > 0)
			{
				load_preset(rightEffects, leftEffects, options[preset_index].c_str());
				right_cur = get_index(rightMode);
				left_cur = get_index(leftMode);
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
			center_window();
			ImGui::Text("This Preset Already Exists! Are You Sure You Want To Overwrite It?");
			if (ImGui::Button("Yes"))
			{
				save_preset(rightEffects, leftEffects, name);
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
			center_window();
			std::vector<std::string> options;
			get_presets(options);
			if (ImGui::InputTextWithHint("Preset Name", "Name", name, IM_ARRAYSIZE(name), ImGuiInputTextFlags_EnterReturnsTrue) && strlen(name) > 0)
			{
				auto is_name = [name](std::string a)
				{
					return a == name;
				};
				if (std::find_if(options.begin(), options.end(), is_name) != options.end())
				{
					printf("Preset already exists!\n");
					save_preset_open = false;
					preset_exists = true;
				}
				else
				{
					save_preset(rightEffects, leftEffects, name);
					save_preset_open = false;
				}
			}

			if ((ImGui::Button("Save")) && strlen(name) > 0)
			{
				auto is_name = [name](std::string a)
				{
					return a == name;
				};
				if (std::find_if(options.begin(), options.end(), is_name) != options.end())
				{
					printf("Preset already exists!\n");
					save_preset_open = false;
					preset_exists = true;
				}
				else
				{
					save_preset(rightEffects, leftEffects, name);
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
			center_window();
			std::vector<std::string> options;
			get_presets(options);
			ImGui::Text("You cannot undo this action!");
			ImGui::Combo("Presets", &preset_index, VectorOfStringGetter, &options, options.size());
			if ((ImGui::Button("Delete!") || SDL_GameControllerGetButton(handle, SDL_CONTROLLER_BUTTON_Y)) && options.size() > 0)
			{
				std::filesystem::remove(std::string(CONFIG_PATH) + options[preset_index] + ".txt");
				delete_preset_open = false;
				preset_index = 0; // prevents blank selection
				ImGui::CloseCurrentPopup();
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
			center_window();
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
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	if (config[1])
	{
		ds::reset_all(handle);
	}
	SDL_GameControllerClose(handle);
	SDL_Quit();
	if (std::filesystem::exists("imgui.ini"))
		std::filesystem::remove("imgui.ini");
	write_config(config, config_size);
	// program termination should free memory I forgot to free :D
	return 0;
}
