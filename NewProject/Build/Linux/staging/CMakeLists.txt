cmake_minimum_required(VERSION 3.10)
project(NewProject)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_options(-fPIC)
file(GLOB_RECURSE ENGINE_SOURCES "/home/rodaqqq/Documents/socka-2dengine/engine/src/*.cpp")
set(APP_SOURCES "/home/rodaqqq/Documents/socka-2dengine/editor/src/GameApp.cpp")
set(VENDOR_SOURCES
  "/home/rodaqqq/Documents/socka-2dengine/vendor/imgui/imgui.cpp"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/imgui/imgui_draw.cpp"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/imgui/imgui_widgets.cpp"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/imgui/imgui_tables.cpp"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/imgui/backends/imgui_impl_sdl2.cpp"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/imgui/backends/imgui_impl_sdlrenderer2.cpp"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/ImGuizmo/ImGuizmo.cpp"
)
add_executable(NewProject ${ENGINE_SOURCES} ${APP_SOURCES} ${VENDOR_SOURCES})
target_include_directories(NewProject PRIVATE
  "/home/rodaqqq/Documents/socka-2dengine/engine/src"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/glm"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/imgui"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/imgui/backends"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/ImGuizmo"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/entt/src"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/sol2/include"
  "/home/rodaqqq/Documents/socka-2dengine/vendor/json/include"
)
find_package(SDL2 REQUIRED)
target_link_libraries(NewProject PRIVATE 
  ${SDL2_LIBRARIES} 
  SDL2_image 
  SDL2_mixer 
  lua5.4 
  pthread 
  dl
)
