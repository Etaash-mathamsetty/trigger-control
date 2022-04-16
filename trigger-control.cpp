#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hidapi/hidapi.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assert.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "icon.h"
#include "crc32.h"
#ifdef __linux__
#include <glib-2.0/glib.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <winuser.h>
#include <shlobj.h>
#include "imgui_impl_win32.h"
#endif
#include <iostream>
#ifdef __linux__
#include <SDL2/SDL_mixer.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <pwd.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <locale>
#include <codecvt>

const char* VERSION = "Version 1.3.2";
char* CONFIG_PATH = new char[PATH_MAX];

const uint8_t seed = 0xA2;
enum dualsense_modes{
    Off = 0x0, //no resistance
    Rigid = 0x1, //continous resistance
    Pulse = 0x2, //section resistance
    Rigid_A = 0x1 | 0x20,
    Rigid_B = 0x1 | 0x04,
    Rigid_AB = 0x1 | 0x20 | 0x04,
    Pulse_A = 0x2 | 0x20,
    Pulse_B = 0x2 | 0x04,
    Pulse_AB = 0x2 | 0x20 | 0x04,
};

void load_preset(uint8_t* outReport,  bool bt, const char* name){
	struct stat info;
	stat(CONFIG_PATH, &info);
	if(info.st_mode & S_IFDIR){
	}
	else{
		#ifdef __linux__
		mkdir(CONFIG_PATH, 0777);
		#endif
		#ifdef _WIN32
		mkdir(CONFIG_PATH);
		#endif
	}
	std::string path = std::string(CONFIG_PATH);
	path += name;
	path += ".txt";
	//std::cout << path << std::endl;
	FILE* f = fopen(path.c_str(), "rb");
	if(f){
		fread(outReport + 11 + bt, sizeof(*outReport), 30 -10, f);
		fclose(f);
	}
	//printf("stub!\n");
}

void save_preset(const uint8_t* outReport, bool bt, const char* name){
	struct stat info;
	stat(CONFIG_PATH, &info);
	if(info.st_mode & S_IFDIR){
	}
	else{
		#ifdef __linux__
		mkdir(CONFIG_PATH, 0777);
		#endif
		#ifdef _WIN32
		mkdir(CONFIG_PATH);
		#endif
	}
	//printf("stub!\n");
	std::string path = std::string(CONFIG_PATH) + name + ".txt";
	//open(path.c_str(), O_RDWR | O_CREAT, 0777);
	FILE* f = fopen(path.c_str(), "wb");
	if(!f)
		return;
	fseek(f, 0, SEEK_SET);
	fwrite(outReport + 11 + bt, sizeof(*outReport), 30 -10, f);
	fclose(f);
}	

void write_config(char* value, size_t size){
	struct stat info;
	stat(CONFIG_PATH, &info);
	if(info.st_mode & S_IFDIR){
	}
	else{
		#ifdef __linux__
		mkdir(CONFIG_PATH, 0777);
		#endif
		#ifdef _WIN32
		mkdir(CONFIG_PATH);
		#endif
	}
	std::string path = std::string(CONFIG_PATH) + "config.ini";
	FILE* f = fopen(path.c_str(), "wb");
	if(!f)
		return;
	fseek(f, 0, SEEK_SET);
	fwrite(value, sizeof(value), size, f);
	fclose(f);
}

void read_config(char** value, size_t size){
	struct stat info;
	stat(CONFIG_PATH, &info);
	if(info.st_mode & S_IFDIR){
	}
	else{
		#ifdef __linux__
		mkdir(CONFIG_PATH, 0777);
		#endif
		#ifdef _WIN32
		mkdir(CONFIG_PATH);
		#endif
	}
	std::string path = std::string(CONFIG_PATH) + "config.ini";
	FILE* f = fopen(path.c_str(), "rb");
	if(!f)
		return;
	fseek(f, 0, SEEK_SET);
	fread(*value, sizeof(**value), size, f);
	fclose(f);
}

void CenteredText(const char* text)
{
	ImVec2 size = ImGui::GetWindowSize();
	ImGui::SetCursorPosX((size.x - ImGui::CalcTextSize(text).x) / 2);
	ImGui::Text(text);
}

void error_sound(){
		#ifdef __linux__
		g_autofree gchar* name = g_build_filename(g_get_user_data_dir(), "sounds","__custom" ,NULL);
		g_autofree gchar* path = g_build_filename(name, "bell-terminal.ogg", NULL);
		Mix_Chunk* sound = Mix_LoadWAV((const char*)path);
		Mix_PlayChannel(-1, sound,0);
		#endif
		#ifdef _WIN32
		MessageBeep(MB_ICONERROR);
		#endif
}

//I spent so long realizing that it was copying the pointer instead of modifying the pointer's address :/
int find_dev(hid_device** handle, bool* bt){
	struct hid_device_info* dev, *cur_dev; 
	dev = hid_enumerate(0x054c, 0x0ce6);
	cur_dev = dev;
	while (cur_dev) {
		if(cur_dev->vendor_id == 0x054c && cur_dev->product_id == 0x0ce6){
			break;
			//printf("found 1\n");
		}
		cur_dev = cur_dev->next;
	}
	if(cur_dev && cur_dev->vendor_id == 0x054c && cur_dev->product_id == 0x0ce6)
		*handle = hid_open_path(cur_dev->path);
	else{
		return -1;
	}
	// wprintf(hid_error(*handle));
	// putchar('\n');
	//printf("interface %d\n", dev->interface_number);
	*bt =  (dev->interface_number == -1);
	hid_free_enumeration(dev);
	
	//printf("%ls\n", hid_error(*handle));
	return 0;
}

int get_mode(int index){
	switch(index){
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
	return 0;
}

int get_index(int mode){
	switch(mode){
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

//this code is satisfying to look at
bool VectorOfStringGetter(void* data, int n, const char** out_text)
{
  const std::vector<std::string>& v = *(std::vector<std::string>*)data;
  *out_text = v[n].c_str();
  return true;
}

int main(int argc, char **argv) {
	memset(CONFIG_PATH, 0, PATH_MAX);
	#ifdef __linux__
	strcpy(CONFIG_PATH, getenv("HOME"));
	strcat(CONFIG_PATH, "/.config/trigger-control/");
	#endif
	#ifdef _WIN32
	strcpy(CONFIG_PATH, getenv("APPDATA"));
	strcat(CONFIG_PATH, "\\trigger-control\\");
	//printf(CONFIG_PATH);
	#endif
	hid_init();
	#ifdef _WIN32
	//ImGui_ImplWin32_EnableDpiAwareness(); this dpi awareness thing does the opposite of what it says
	#endif
	const size_t config_size = 10;
	char*config = (char*)alloca(sizeof(char) * config_size);
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO);
	uint32_t WindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	SDL_Window *window = SDL_CreateWindow("Trigger Controls", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 520, WindowFlags);
	SDL_SetWindowMinimumSize(window, 300, 250);
	int height = 520, width = 640;
	assert(window);
	SDL_GLContext context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, context);
	SDL_Surface* surface;
	surface = SDL_CreateRGBSurfaceWithFormatFrom(gimp_image.pixel_data, gimp_image.width, gimp_image.height, gimp_image.bytes_per_pixel * 8, 4 *  gimp_image.width,SDL_PIXELFORMAT_RGBA32 );
	SDL_SetWindowIcon(window, surface);
	  SDL_FreeSurface(surface);
	  #ifdef __linux__
	 if( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 )  < 0){
		 //error_sound(); lmao cannot play the sound without mixer
		 SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"ERROR","could not initialize sdl_mixer",window);
		 exit(EXIT_FAILURE);
	 }
	 #endif
	glewExperimental=true;
	glewInit();
	bool popup_open = false;
	bool save_preset_open = false;
	bool load_preset_open = false;
	bool delete_preset_open = false;
	bool preset_exists = false;
	bool options_open = false;
			char name[100];	
			//name.reserve(100);
	IMGUI_CHECKVERSION();
	    ImGui::CreateContext();
	    ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
		read_config(&config, config_size);
		if(config[0] == 1)
	    ImGui::StyleColorsDark();
		else
		ImGui::StyleColorsLight();

		ImGui::GetStyle().WindowRounding = 5.0f;
		ImGui::GetStyle().PopupRounding = 5.0f;
		ImGui::GetStyle().FrameRounding = 5.0f;
		ImGui::GetStyle().GrabRounding = 5.0f;
	       // setup platform/renderer bindings
	    ImGui_ImplSDL2_InitForOpenGL(window, context);
	    ImGui_ImplOpenGL3_Init("#version 150");
	    SDL_GL_SetSwapInterval(1);
		//float dpi_scaling = 1.0f;
		#ifdef _WIN32
		float dpi_x, dpi_y, dpi_z;
		SDL_GetDisplayDPI(0, &dpi_x, &dpi_y, &dpi_z);
		float dpi_scaling = dpi_x / 96.0f;
		//std::cout << dpi_scaling << std::endl;
		//SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,"DPI",std::to_string(dpi_scaling).c_str(),window);
		ImGui::GetStyle().ScaleAllSizes(dpi_scaling);
		io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 48.0f * dpi_scaling);
		io.FontGlobalScale = 0.5f;
		#endif
		bool bt = false;
		//char* path = NULL;
		int preset_index = 0;
		//here for potential future multi-controller support (yes, I know I can use hid_open, but it only works for a single controller)
		hid_device* handle;
		int res = find_dev(&handle, &bt);
		if(res == -1){
			error_sound();
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"ERROR","could not find a dualsense controller!",window);
			exit(EXIT_FAILURE);
		}
		//printf("%d\n",bt);
		//free(path);
		bool running = true;
		//SDL_SetWindowResizable(window, SDL_bool::SDL_TRUE);
		uint8_t* outReport = new uint8_t[78];
	    memset(outReport, 0, 78);
	    if(!bt){
	   		    outReport[0] = 0x2;
	   		    outReport[1] = 0x04 | 0x08;
	   		    outReport[2] = 0x40;
	   		    }
	   		    if(bt){
	   		    outReport[0] = 0x31; //thx ds4windows
	   		    outReport[1] = 0x2;
	   		    outReport[2] = 0x04 | 0x08;
	   			outReport[3] = 0x40;
	   		    }
		const char* states[9] = {"Off","Rigid","Pulse","RigidA","RigidB","RigidAB","PulseA","PulseB","PulseAB"};
		int left_cur = 0;
		int right_cur = 0;
	while(running){

		 SDL_Event event;
		    while (SDL_PollEvent(&event))
		    {
		    	 ImGui_ImplSDL2_ProcessEvent(&event);
		    	if (event.type == SDL_QUIT)
		        {
		    	   running = false;
		        }
		    	if(event.window.event == SDL_WINDOWEVENT_RESIZED){
		    		SDL_GetWindowSize(window, &width, &height);
					glViewport(0, 0, width, height);
		    	}
		    }
		const wchar_t* error = hid_error(handle);
		if(wcscmp(error, L"Success") != 0){
			#ifdef __linux__
			sleep(1);
			#endif
			#ifdef _WIN32
			Sleep(1000);
			#endif
			//printf("here!\n");
			hid_close(handle);
			int res = find_dev(&handle, &bt);
			if(res == -1){
			error_sound();
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"ERROR","Controller Disconnected! (or something else happened)",window);
			exit(EXIT_FAILURE);
			}
		}

	    glClearColor(0.f, 0.f, 0.f, 0.f);
	    glClear(GL_COLOR_BUFFER_BIT);
	    ImGui_ImplOpenGL3_NewFrame();
	    ImGui_ImplSDL2_NewFrame(window);
	    ImGui::NewFrame();
		    ImGui::SetNextWindowSize(
            ImVec2(float(width), float(height)),
            ImGuiCond_Always
            );	
        ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Always, ImVec2(0,0));
		//ImGui::ShowDemoWindow();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
	    ImGui::Begin("Controls", NULL,ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings);
		ImGui::PopStyleVar();
	//	ImGui::ShowDemoWindow();
	    if(ImGui::Checkbox("Using Bluetooth?",&bt)){
			if(bt){
				memmove(&outReport[12], &outReport[11] , 30 -10);
			}
			else{
				memmove(&outReport[11], &outReport[12] , 30 -10);
			}
		}
		if(ImGui::BeginMenuBar()){
		if(ImGui::BeginMenu("File")){
			if(ImGui::MenuItem("Load Preset")){
					load_preset_open = true;
					memset(name, 0, sizeof(name));
					//name.clear();
			}
			if(ImGui::MenuItem("Save Preset")){
					save_preset_open = true;
					memset(name, 0, sizeof(name));
					//name.clear();
			}
			if(ImGui::MenuItem("Delete Preset")){
					delete_preset_open = true;
					memset(name, 0, sizeof(name));
					//name.clear();
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Tools")){
			if(ImGui::MenuItem("Options")){
				options_open = true;
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Help")){
			if(ImGui::MenuItem("About")){
					popup_open = true;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
		}
		if(popup_open)
		{
			//ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing);
			ImGui::OpenPopup("About");
		}
		if(load_preset_open ){
			ImGui::OpenPopup("Load Preset");
		}
		if(save_preset_open){
			ImGui::OpenPopup("Save Preset");
		}
		if(delete_preset_open){
			ImGui::OpenPopup("Delete Preset");
		}
		if(options_open){
			ImGui::OpenPopup("Options");
		}

		if(ImGui::BeginPopupModal("About", &popup_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)){
					//printf("about2!\n");
				ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
				_pos.x -= ImGui::GetWindowWidth()/2;
				_pos.y -= ImGui::GetWindowHeight()/2;
				ImGui::SetWindowPos(_pos);
				CenteredText("Trigger Control");
				CenteredText(VERSION);
				ImGui::Separator();
				ImGui::Text("Made with FOSS, Powered by SDL2");
				ImGui::EndPopup();
		}
		if(ImGui::BeginPopupModal("Load Preset", &load_preset_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)){
			ImGui::SetWindowSize(ImVec2(300,80),ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth()/2;
			_pos.y -= ImGui::GetWindowHeight()/2;
			ImGui::SetWindowPos(_pos);
			std::vector<std::string> options;
			#ifdef __linux__
			for(const auto& file : std::filesystem::directory_iterator(CONFIG_PATH)){
				std::string filename = file.path().filename();
				filename = filename.substr(0,filename.find_last_of("."));
				if(file.path().extension() == ".txt"){
					options.push_back(filename);
				}
				//options.push_back(filename);
			}
			#endif
			#ifdef _WIN32
			for(const auto& file : std::filesystem::directory_iterator(CONFIG_PATH)){
				std::wstring str = file.path().filename();
				std::string str2(str.begin(), str.end());
				str2 = str2.substr(0,str2.find_last_of("."));
				if(file.path().extension() == L".txt"){
					options.push_back(str2);
				}
			//	options.push_back(str2);
			}
			#endif
			ImGui::Combo("Presets", &preset_index, VectorOfStringGetter, &options, options.size());
			if(ImGui::Button("Load") && options.size() > 0){
				load_preset(outReport, bt, options[preset_index].c_str());
				right_cur = get_index(outReport[11 + bt]);
				left_cur = get_index(outReport[22+ bt]);
				if(!bt)
				hid_write(handle,outReport,65);
				else{
				unsigned int crc = crc32_le(UINT32_MAX, &seed, 1);
				crc = ~crc32_le(crc, outReport, 74);
				printf("crc: %u\n", crc);
                outReport[74] = (uint8_t)crc;
                outReport[75] = (uint8_t)(crc >> 8);
                outReport[76] = (uint8_t)(crc >> 16);
                outReport[77] = (uint8_t)(crc >> 24);
				hid_write(handle,outReport, 78);
			}
				load_preset_open = false;
			}

			ImGui::EndPopup();
		}

		if(preset_exists){
			ImGui::OpenPopup("Preset Exists");
		}

		if(ImGui::BeginPopup("Preset Exists", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)){
			ImGui::SetWindowSize(ImVec2(300,180),ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth()/2;
			_pos.y -= ImGui::GetWindowHeight()/2;
			ImGui::SetWindowPos(_pos);
			ImGui::Text("This Preset already exists! Are You Sure You Want To Overwrite It?");
			if(ImGui::Button("Yes")){
				save_preset(outReport, bt, name);
				save_preset_open = false;
				ImGui::CloseCurrentPopup();
				preset_exists = false;
			}
			ImGui::SameLine();
			if(ImGui::Button("No")){
				ImGui::CloseCurrentPopup();
				preset_exists = false;
				save_preset_open = true;
			}
			ImGui::EndPopup();
		}

		if(ImGui::BeginPopupModal("Save Preset", &save_preset_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)){
			ImGui::SetWindowSize(ImVec2(300,80),ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth()/2;
			_pos.y -= ImGui::GetWindowHeight()/2;
			ImGui::SetWindowPos(_pos);
			std::vector<std::string> options;
			#ifdef __linux__
			for(const auto& file : std::filesystem::directory_iterator(CONFIG_PATH)){
				std::string filename = file.path().filename();
				filename = filename.substr(0,filename.find_last_of("."));
				if(file.path().extension() == ".txt"){
					options.push_back(filename);
				}
				//options.push_back(filename);
			}
			#endif
			#ifdef _WIN32
			//windows, you are always difficult...
			for(const auto& file : std::filesystem::directory_iterator(CONFIG_PATH)){
				std::wstring str = file.path().filename();
				std::string str2(str.begin(), str.end());
				str2 = str2.substr(0,str2.find_last_of("."));
				if(file.path().extension() == L".txt"){
					options.push_back(str2);
				}
				//options.push_back(str2);
			}

			#endif
			if(ImGui::InputTextWithHint("Preset Name", "Name",name, IM_ARRAYSIZE(name), ImGuiInputTextFlags_EnterReturnsTrue) && name[0] != '\0'){
				auto is_name = [name](std::string a){
					return strcmp(a.c_str(), name) == 0; //remove trailing characters we left behind lol
				};
				if(std::find_if(options.begin(), options.end(), is_name) != options.end()){
					printf("Preset already exists!\n");
					//ImGui::CloseCurrentPopup();
					save_preset_open = false;
					//ImGui::OpenPopup("Preset Exists!");
					preset_exists = true;
				}
				else{
					save_preset(outReport, bt, name);
					save_preset_open = false;
				}
			}

			if(ImGui::Button("Save") && name[0] != '\0'){
				auto is_name = [name](std::string a){
					return strcmp(a.c_str(), name) == 0; //remove trailing characters that we left behind lol
				};
				if(std::find_if(options.begin(), options.end(), is_name) != options.end()){
					printf("Preset already exists!\n");
					//preset_exists = true;
					//ImGui::CloseCurrentPopup();
					save_preset_open = false;
					//ImGui::OpenPopup("Preset Exists!");
					preset_exists = true;
				}
				else{
					save_preset(outReport, bt, name);
					save_preset_open = false;
				}
			}

			ImGui::EndPopup();
		}

		if(ImGui::BeginPopupModal("Delete Preset", &delete_preset_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)){
			ImGui::SetWindowSize(ImVec2(300,80),ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth()/2;
			_pos.y -= ImGui::GetWindowHeight()/2;
			ImGui::SetWindowPos(_pos);
			std::vector<std::string> options;
			#ifdef __linux__
			for(const auto& file : std::filesystem::directory_iterator(CONFIG_PATH)){
				std::string filename = file.path().filename();
				filename = filename.substr(0,filename.find_last_of("."));
				if(file.path().extension() == ".txt"){
					options.push_back(filename);
				}
				//options.push_back(filename);
			}
			#endif
			#ifdef _WIN32
			//windows, you are always difficult...
			for(const auto& file : std::filesystem::directory_iterator(CONFIG_PATH)){
				std::wstring str = file.path().filename();
				std::string str2(str.begin(), str.end());
				str2 = str2.substr(0,str2.find_last_of("."));
				if(file.path().extension() == L".txt"){
					options.push_back(str2);
				}
				//options.push_back(str2.c_str());
			}
			#endif
			ImGui::Combo("Presets", &preset_index, VectorOfStringGetter, &options, options.size());
			if(ImGui::Button("Delete!") && options.size() > 0){
				remove((std::string(CONFIG_PATH)+options[preset_index]+".txt").c_str());
				delete_preset_open = false;
			}
			ImGui::EndPopup();
		}

		if(ImGui::BeginPopupModal("Options", &options_open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)){
			ImGui::SetWindowSize(ImVec2(500,100),ImGuiCond_Always);
			ImVec2 _pos = ImGui::GetMainViewport()->GetCenter();
			_pos.x -= ImGui::GetWindowWidth()/2;
			_pos.y -= ImGui::GetWindowHeight()/2;
			ImGui::SetWindowPos(_pos);
			//ImGui::Text("It's Empty here :(");
		
			if(ImGui::Button("Light Mode", ImVec2(ImGui::GetWindowWidth()/2-10,25))){
				ImGui::StyleColorsLight();
				config[0] = 0;
			}
			ImGui::SameLine();
			//fix spacing between edge and button
			if(ImGui::Button("Dark Mode", ImVec2(ImGui::GetWindowWidth()/2-14,25))){
				ImGui::StyleColorsDark();
				config[0] = 1;
			}
			
			if(ImGui::Button("Close")){
				options_open = false;
			}

			ImGui::EndPopup();
		}
	    if(ImGui::Button("Reset")){
		    memset(outReport, 0, 78);
		    if(!bt){
		    outReport[0] = 0x2;
		    outReport[1] = 0x04 | 0x08;
		    outReport[2] = 0x40;
		    }
		    if(bt){
		    outReport[0] = 0x31; //thx ds4windows
		    outReport[1] = 0x2;
		    outReport[2] = 0x04 | 0x08;
			outReport[3] = 0x40;
		    }
		    outReport[11 + bt] = (uint8_t)dualsense_modes::Rigid_B;
		    outReport[22 + bt] = (uint8_t)dualsense_modes::Rigid_B;
			if(!bt)
			hid_write(handle,outReport,65);
			else{

				unsigned int crc = crc32_le(UINT32_MAX, &seed, 1);
				crc = ~crc32_le(crc, outReport, 74);
				printf("crc: %u\n", crc);
                outReport[74] = (uint8_t)crc;
                outReport[75] = (uint8_t)(crc >> 8);
                outReport[76] = (uint8_t)(crc >> 16);
                outReport[77] = (uint8_t)(crc >> 24);
				hid_write(handle,outReport, 78);

			}
			left_cur = 0;
			right_cur = 0;
			printf("reset!\n");
		    outReport[11 + bt] = (uint8_t)0;
		    outReport[22 + bt] = (uint8_t)0;
	    }

	    ImGui::Text("Right Trigger:");
	    ImGui::Combo("Right Mode", &right_cur, states, IM_ARRAYSIZE(states));
	    uint8_t min  = 0;
	    uint8_t max = UINT8_MAX;
	    outReport[11 + bt] = get_mode(right_cur);
	    ImGui::SliderScalar("Right Start Resistance", ImGuiDataType_U8, &outReport[12+ bt] ,&min, &max, "%d",0);
	    ImGui::SliderScalar("Right Effect Force", ImGuiDataType_U8,&outReport[13+ bt] , &min, &max, "%d", 0);
	    ImGui::SliderScalar("Right Range Force", ImGuiDataType_U8,&outReport[14+ bt], &min, &max, "%d", 0);
	    ImGui::SliderScalar("Right Near Release Strength", ImGuiDataType_U8,&outReport[15+ bt],&min, &max, "%d", 0);
	    ImGui::SliderScalar("Right Near Middle Strength",ImGuiDataType_U8, &outReport[16+ bt], &min, &max, "%d", 0);
	    ImGui::SliderScalar("Right Pressed Strength", ImGuiDataType_U8,&outReport[17+ bt], &min, &max, "%d", 0);
	    ImGui::SliderScalar("Right Actuation Frequency", ImGuiDataType_U8,&outReport[20+ bt], &min, &max, "%d", 0);
	    ImGui::Text("Left Trigger:");
	    ImGui::Combo("Left Mode", &left_cur, states, IM_ARRAYSIZE(states));
	    outReport[22 + bt] = get_mode(left_cur);
	    ImGui::SliderScalar("Left Start Resistance", ImGuiDataType_U8, &outReport[23+ bt] ,&min, &max, "%d",0);
	    ImGui::SliderScalar("Left Effect Force", ImGuiDataType_U8,&outReport[24+ bt] , &min, &max, "%d", 0);
	    ImGui::SliderScalar("Left Range Force", ImGuiDataType_U8,&outReport[25+ bt], &min, &max, "%d", 0);
	    ImGui::SliderScalar("Left Near Release Strength", ImGuiDataType_U8,&outReport[26+ bt],&min, &max, "%d", 0);
	    ImGui::SliderScalar("Left Near Middle Strength",ImGuiDataType_U8, &outReport[27+ bt], &min, &max, "%d", 0);
	    ImGui::SliderScalar("Left Pressed Strength", ImGuiDataType_U8,&outReport[28+ bt], &min, &max, "%d", 0);
	    ImGui::SliderScalar("Left Actuation Frequency", ImGuiDataType_U8,&outReport[30+ bt], &min, &max, "%d", 0);
	    if(ImGui::Button("Apply")){
	    	printf("applied! bt: %d\n", bt);
	    	 if(!bt){
	    			    outReport[0] = 0x2;
	    			    outReport[1] = 0x04 | 0x08;
	    			    outReport[2] = 0x40;
	    			    }
	    			    if(bt){
	    			    outReport[0] = 0x31; //thx ds4windows
	    			    outReport[1] = 0x2;
	    			    outReport[2] = 0x04 | 0x08;
	    				outReport[3] = 0x40;
	    			    }
	    	if(!bt)
	    	hid_write(handle,outReport,65);
	    	else{
	    		unsigned int crc = crc32_le(UINT32_MAX, &seed, 1);
	    						crc = ~crc32_le(crc, outReport, 74);
	    						printf("crc: %u\n", crc);
                outReport[74] = (uint8_t)crc;
                outReport[75] = (uint8_t)(crc >> 8);
                outReport[76] = (uint8_t)(crc >> 16);
                outReport[77] = (uint8_t)(crc >> 24);
                hid_write(handle, outReport,78);
	    	}
	    }

	    ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);

	}
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

#ifdef __linux__
	Mix_CloseAudio();
	Mix_Quit();
#endif
	hid_close(handle);
	hid_exit();
	delete outReport;
	SDL_Quit();
	remove("imgui.ini");
	write_config(config, config_size);
	//program termination should free memory I forgot to free :D
	return 0;
}
