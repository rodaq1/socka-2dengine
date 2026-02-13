add_library(engine STATIC
    src/core/Application.cpp
    src/core/Log.cpp
    src/core/Time.cpp
    src/core/Input.cpp
    src/ecs/Entity.cpp
    src/scene/Scene.cpp
    src/core/AssetManager.cpp
    src/ecs/systems/RendererSystem.cpp
    src/ecs/System.cpp
    src/ecs/systems/MovementSystem.cpp
    src/ecs/systems/InputSystem.cpp
    src/ecs/systems/CollisionSystem.cpp
    src/ecs/systems/PhysicsSystem.cpp
    src/ecs/systems/CameraSystem.cpp
    src/ecs/systems/ScriptSystem.cpp
    src/core/FileWatcher.cpp
    src/core/ProjectSerializer.cpp
    src/ecs/components/InheritanceComponent.cpp
    src/ecs/systems/SoundSystem.cpp
    src/core/Camera.cpp
    src/core/Font.cpp
    src/core/FontManager.cpp
    src/ecs/systems/TextSystem.cpp
    )

if(MINGW)
    target_compile_options(engine PRIVATE "-Wa,-mbig-obj")
endif()

target_include_directories(engine PUBLIC
    ${ALL_SDL_INCLUDES}
    ${LUA_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/vendor/sol2/include
    ${CMAKE_SOURCE_DIR}/vendor/glm
    ${CMAKE_SOURCE_DIR}/vendor/entt/src
    ${CMAKE_SOURCE_DIR}/vendor/json/include
    ${CMAKE_SOURCE_DIR}/engine/src
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
    ${CMAKE_SOURCE_DIR}/vendor/ImGuizmo
)

target_link_libraries(engine
    PUBLIC
    imgui
    ${SDL2_LIBS}
    ${SDL2_IMAGE_LIBS}
    ${SDL2_MIXER_LIBS}
    ${SDL2_TTF_LIBS}
    ${LUA_LIBS}
)