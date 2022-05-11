PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS = trigger-control.o imgui/imgui.o imgui/imgui_impl_sdlrenderer.o imgui/imgui_impl_sdl.o imgui/imgui_demo.o imgui/imgui_draw.o imgui/imgui_tables.o imgui/imgui_widgets.o imgui/ImGuiFileDialog.o
LIBRARIES = `pkg-config --libs --cflags SDL2_mixer` `pkg-config --libs --cflags glib-2.0` 

CFLAGS = -Wall -std=c++17  
LDFLAGS = $(LIBRARIES)

all:	run

run:		CFLAGS += -O2
run:		trigger-control

debug:	CFLAGS += -DDEBUG -g
debug: 	trigger-control

trigger-control:	$(OBJS)
	$(CXX) -o $@ $^  $(LDFLAGS) 
	$(EXTRA_CMDS)

%.o:	%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $<

%.o:	%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -fr trigger-control trigger-control.exe imgui/imgui_impl_win32.o  $(OBJS)
