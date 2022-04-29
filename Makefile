PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS = trigger-control.o imgui.o imgui_impl_opengl3.o imgui_impl_sdl.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o
LIBRARIES = `pkg-config --libs --cflags SDL2_mixer` `pkg-config --cflags --libs glew` `pkg-config --libs --cflags glib-2.0` -Wl,--no-as-needed -ldl

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

%.o:	$(PROJECT_ROOT)%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $<

%.o:	$(PROJECT_ROOT)%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	rm -fr trigger-control trigger-control.exe imgui_impl_win32.o  $(OBJS)
