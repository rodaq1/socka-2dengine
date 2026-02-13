add_library(imgui_backends STATIC
${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl2.cpp
${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdlrenderer2.cpp
${CMAKE_SOURCE_DIR}/vendor/ImGuizmo/ImGuizmo.cpp
)

target_include_directories(imgui_backends PRIVATE
${CMAKE_SOURCE_DIR}/vendor/imgui
${CMAKE_SOURCE_DIR}/vendor/imgui/backends
${SDL2_INCLUDE_DIRS}
${CMAKE_SOURCE_DIR}/vendor/ImGuizmo
)

target_link_libraries(imgui_backends PRIVATE ${SDL2_LIBS})


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
)


add_executable(Runtime src/runtime.cpp)


foreach(target editor Runtime)

    if(WIN32) # Standard way to check for Windows/MinGW
        target_link_libraries(${target} PRIVATE
            engine
            imgui_backends
            imgui
            mingw32
            ${SDL2_MAIN_LIB}
            ${SDL2_LIBS}
            ${SDL2_IMAGE_LIBS}
            ${SDL2_MIXER_LIBS}
            ${SDL2_TTF_LIBS}
            comdlg32 ole32 shell32 shlwapi 
            ${LUA_LIBS}   
            m   
        )

        target_include_directories(${target} PRIVATE
            ${CMAKE_SOURCE_DIR}/vendor/pfd
            ${LUA_INCLUDE_DIR} 
        )

        set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE TRUE)

    else()
        target_link_libraries(${target} PRIVATE
            engine
            imgui_backends
            imgui
            ${SDL2_LIBS}
            ${SDL2_IMAGE_LIBS}
            ${SDL2_MIXER_LIBS}
            ${SDL2_TTF_LIBS}
            ${LUA_LIBS}
        )
    endif()

    # Common includes for all platforms
    target_include_directories(${target} PRIVATE
        ${CMAKE_SOURCE_DIR}/engine/src
        ${CMAKE_SOURCE_DIR}/vendor/ImGuizmo
        ${CMAKE_SOURCE_DIR}/vendor/glm
        ${CMAKE_SOURCE_DIR}/vendor/imgui
        ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
        ${CMAKE_SOURCE_DIR}/vendor/entt/single_include
        ${CMAKE_SOURCE_DIR}/vendor/json/include
        ${CMAKE_SOURCE_DIR}/vendor/sol2/include
        ${CMAKE_SOURCE_DIR}/vendor/pfd
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIR}
    )

endforeach()
