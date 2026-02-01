cmake_minimum_required(VERSION 3.16)
project(socka-2dengine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# We use target_compile_definitions inside the engine subdirectory instead
# to ensure the flags are bound directly to the engine target.

find_package(PkgConfig REQUIRED)

# Dependencies
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
find_package(Lua REQUIRED)

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor/sol2/include
    ${CMAKE_SOURCE_DIR}/vendor/glm
    ${CMAKE_SOURCE_DIR}/vendor/entt/src
    ${CMAKE_SOURCE_DIR}/engine/src
    ${CMAKE_SOURCE_DIR}/vendor/ImGuizmo
    ${CMAKE_SOURCE_DIR}/vendor/pfd
    ${CMAKE_SOURCE_DIR}/vendor/json/include
)

# Build ImGui Static
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_demo.cpp
)

target_include_directories(imgui PUBLIC vendor/imgui)

# Subdirectories
add_subdirectory(engine)
add_subdirectory(editor)