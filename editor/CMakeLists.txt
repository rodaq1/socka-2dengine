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
    if(WIN32)
        target_link_libraries(${target} PRIVATE
            engine
            imgui_backends
            imgui
            mingw32        # Must be first for SDL2
            SDL2main       # Must be before SDL2
            ${SDL2_LIBS}   # This is your SDL2.dll.a
            ${SDL2_IMAGE_LIBS}
            ${SDL2_MIXER_LIBS}
            comdlg32 ole32 shell32 shlwapi # Windows UI systems for pfd
            /usr/x86_64-w64-mingw32/lib/liblua.dll.a
            m
            
        )
        
        # This fixes the _setjmp error by ensuring we use the correct jump version
        target_compile_definitions(${target} PRIVATE LUA_USE_APICHECK) 
        target_compile_definitions(${target} PRIVATE LUA_USE_WINDOWS)
    else()
        # Your existing Linux link logic
        target_link_libraries(${target} PRIVATE
            engine imgui_backends imgui ${SDL2_LIBS} 
            ${SDL2_IMAGE_LIBS} ${SDL2_MIXER_LIBS} ${LUA_LIBRARIES}
        )
    endif()

    target_include_directories(${target} PRIVATE 
        ${CMAKE_SOURCE_DIR}/engine/src
        ${CMAKE_SOURCE_DIR}/vendor/ImGuizmo
        ${CMAKE_SOURCE_DIR}/vendor/glm
        ${CMAKE_SOURCE_DIR}/vendor/imgui
        ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
        ${CMAKE_SOURCE_DIR}/vendor/pfd                 # Corrected name
        ${CMAKE_SOURCE_DIR}/vendor/entt/single_include # Usually where entt.hpp lives
        ${CMAKE_SOURCE_DIR}/vendor/json/include        # Path for nlohmann/json.hpp
        ${CMAKE_SOURCE_DIR}/vendor/sol2/include
        /usr/x86_64-w64-mingw32/include 
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
    )

    if(WIN32)
        set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endforeach()