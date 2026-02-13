cmake_minimum_required(VERSION 3.16)
project(socka-2dengine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    message(STATUS "Using Windows/MinGW dependency resolution")

    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_mixer REQUIRED)
    find_package(SDL2_ttf REQUIRED)
    find_package(Lua REQUIRED)

    set(SDL2_LIBS SDL2::SDL2)
    set(SDL2_IMAGE_LIBS SDL2_image::SDL2_image)
    set(SDL2_MIXER_LIBS SDL2_mixer::SDL2_mixer)
    set(SDL2_TTF_LIBS SDL2_ttf::SDL2_ttf)
    set(LUA_LIBS ${LUA_LIBRARIES})

    set(ALL_SDL_INCLUDES 
        ${SDL2_INCLUDE_DIR} 
        ${SDL2_IMAGE_INCLUDE_DIR} 
        ${SDL2_MIXER_INCLUDE_DIR} 
        ${SDL2_TTF_INCLUDE_DIR}
    )

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic")

else()
    message(STATUS "Using Linux PkgConfig resolution")

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
    pkg_check_modules(LUA REQUIRED lua54)

    set(SDL2_LIBS ${SDL2_LIBRARIES})
    set(SDL2_IMAGE_LIBS ${SDL2_IMAGE_LIBRARIES})
    set(SDL2_MIXER_LIBS ${SDL2_MIXER_LIBRARIES})
    set(SDL2_TTF_LIBS ${SDL2_TTF_LIBRARIES})
    set(LUA_LIBS ${LUA_LIBRARIES})
    
    set(ALL_SDL_INCLUDES 
        ${SDL2_INCLUDE_DIRS} 
        ${SDL2_IMAGE_INCLUDE_DIRS} 
        ${SDL2_MIXER_INCLUDE_DIRS} 
        ${SDL2_TTF_INCLUDE_DIRS}
    )
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