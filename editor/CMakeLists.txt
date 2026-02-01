add_executable(editor
    src/EditorApp.cpp
    src/main.cpp
    src/ui/Console.cpp
    src/ui/Inspector.cpp
    src/ui/AssetManager.cpp
    src/ui/Hierarchy.cpp
    src/ui/MainMenu.cpp
    src/ui/Viewport.cpp
    src/ui/ProjectBrowser.cpp
    src/ui/Gizmos.cpp

    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdlrenderer2.cpp
    ${CMAKE_SOURCE_DIR}/vendor/ImGuizmo/ImGuizmo.cpp
)

add_executable(Runtime
    src/runtime.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdlrenderer2.cpp
    ${CMAKE_SOURCE_DIR}/vendor/ImGuizmo/ImGuizmo.cpp
)

target_include_directories(editor PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
)
target_include_directories(Runtime PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
)

target_link_libraries(editor PUBLIC
    engine
    imgui
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
)

target_link_libraries(Runtime PUBLIC
    engine
    imgui
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
)