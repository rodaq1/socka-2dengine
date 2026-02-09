cmake_minimum_required(VERSION 3.16)
project(socka-2dengine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32) # Works for both cross-compiling and native Windows
    message(STATUS "Using Windows/MinGW dependency resolution")

    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    
    # Let CMake find Lua instead of hardcoding /usr/
    find_package(Lua REQUIRED)

    set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
    set(SDL2_LIBS SDL2::SDL2)
    set(SDL2_MAIN_LIB SDL2::SDL2main)
    set(SDL2_IMAGE_LIBS SDL2_image::SDL2_image)
    set(SDL2_MIXER_LIBS SDL2_mixer::SDL2_mixer)
    set(LUA_LIBS ${LUA_LIBRARIES})
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic")

else()
    message(STATUS "Using Linux PkgConfig resolution")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
    pkg_check_modules(LUA REQUIRED lua54)

    set(SDL2_LIBS ${SDL2_LIBRARIES})
    set(SDL2_IMAGE_LIBS ${SDL2_IMAGE_LIBRARIES})
    set(SDL2_MIXER_LIBS ${SDL2_MIXER_LIBRARIES})
    set(LUA_LIBS ${LUA_LIBRARIES})
endif()

add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_demo.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/vendor/imgui
)

add_subdirectory(engine)
add_subdirectory(editor)
