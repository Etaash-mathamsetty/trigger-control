PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS = trigger-control.o imgui/imgui.o imgui/imgui_impl_sdlrenderer.o imgui/imgui_impl_sdl.o imgui/imgui_demo.o imgui/imgui_impl_win32.o imgui/imgui_draw.o imgui/imgui_tables.o imgui/imgui_widgets.o
LIBRARIES = `pkg-config --libs --cflags SDL2_mixer` `pkg-config --libs --cflags glib-2.0` 

CFLAGS = -Wall -std=c++17  

all:	run

run:		CFLAGS += -O2
run:		trigger-control

debug:	CFLAGS += -DDEBUG -g
debug: 	trigger-control

windows: CROSS=x86_64-w64-mingw32
windows: LIBRARIES = -lmingw32 -lopengl32 -luuid -lgdi32 -lcomdlg32 -ldwmapi `$(CROSS)-pkg-config --libs --cflags sdl2`
windows: CXX=$(CROSS)-g++
windows: CC=$(CROSS)-gcc
windows: trigger-control

LDFLAGS = $(LIBRARIES)


trigger-control:	$(OBJS)
	$(CXX) -o $@ $^  $(LDFLAGS) 
	$(EXTRA_CMDS)

%.o:	%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $<

%.o:	%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $<

clean:
	rm -fr trigger-control trigger-control.exe imgui/imgui_impl_win32.o  $(OBJS)
