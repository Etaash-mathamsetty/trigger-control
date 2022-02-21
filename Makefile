PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

OBJS = trigger-control.o imgui.o imgui_impl_opengl3.o imgui_impl_sdl.o imgui_demo.o imgui_draw.o imgui_tables.o imgui_widgets.o
LIBRARIES = -lusb -lSDL2 -lhidapi-hidraw -lGL -lGLEW -I/usr/include/SDL2

ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g
	LDFLAGS += $(LIBRARIES)
else ifeq ($(BUILD_MODE),run)
	CFLAGS += -O2
	LDFLAGS += $(LIBRARIES)
else ifeq ($(BUILD_MODE),linuxtools)
	CFLAGS += -g -pg -fprofile-arcs -ftest-coverage $(LIBRARIES)
	LDFLAGS += -pg -fprofile-arcs -ftest-coverage $(LIBRARIES)
	EXTRA_CLEAN += trigger-control.gcda trigger-control.gcno $(PROJECT_ROOT)gmon.out
	EXTRA_CMDS = rm -rf trigger-control.gcda
else
    $(error Build mode $(BUILD_MODE) not supported by this Makefile)
endif

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
