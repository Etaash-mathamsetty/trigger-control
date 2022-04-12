PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS = trigger-control.o imgui.o imgui_impl_opengl3.o imgui_impl_sdl.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o
LIBRARIES = `pkg-config --cflags --libs libusb` `pkg-config --libs --cflags SDL2_mixer` `pkg-config --cflags --libs hidapi-hidraw` `pkg-config --cflags --libs glew` `pkg-config --libs --cflags glib-2.0` -Wl,--no-as-needed -ldl

CFLAGS = -O2
LDFLAGS = $(LIBRARIES)

all:	trigger-control

trigger-control:	$(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^
	$(EXTRA_CMDS)

%.o:	$(PROJECT_ROOT)%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $<

%.o:	$(PROJECT_ROOT)%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	rm -fr trigger-control $(OBJS) $(EXTRA_CLEAN)
