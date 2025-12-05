add_executable(editor
    src/EditorApp.cpp
    src/main.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl2.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdlrenderer2.cpp
)

target_include_directories(editor PRIVATE
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