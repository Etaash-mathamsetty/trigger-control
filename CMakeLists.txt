cmake_minimum_required(VERSION 3.0)
find_package(PkgConfig)

project(trigger-control)

pkg_check_modules(SDL2 REQUIRED sdl2)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    pkg_check_modules(DBUS REQUIRED dbus-1)
    pkg_check_modules(NOTIFY REQUIRED libnotify)
endif()

add_executable(trigger-control trigger-control.cpp ${CMAKE_SOURCE_DIR}/imgui/imgui.cpp ${CMAKE_SOURCE_DIR}/imgui/imgui_widgets.cpp 
${CMAKE_SOURCE_DIR}/imgui/imgui_demo.cpp ${CMAKE_SOURCE_DIR}/imgui/imgui_draw.cpp ${CMAKE_SOURCE_DIR}/imgui/imgui_impl_sdl.cpp 
${CMAKE_SOURCE_DIR}/imgui/imgui_impl_sdlrenderer.cpp ${CMAKE_SOURCE_DIR}/imgui/imgui_tables.cpp ${CMAKE_SOURCE_DIR}/imgui/imgui_impl_win32.cpp 
${CMAKE_SOURCE_DIR}/libtrigger-control/ds-trigger-control.cpp)

target_compile_features(trigger-control PUBLIC cxx_std_17)
target_include_directories(trigger-control PUBLIC ${DBUS_INCLUDE_DIRS} ${NOTIFY_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} 
${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR}/libtrigger-control)

target_link_libraries(trigger-control ${DBUS_LIBRARIES} ${NOTIFY_LIBRARIES} ${SDL2_LIBRARIES} ${WIN_LIBRARIES})