# engine/CMakeLists.txt

add_library(engine STATIC
    #source files
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
)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor/glm
    ${CMAKE_SOURCE_DIR}/vendor/entt/src/
    ${LUA_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/vendor/sol2/include
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
    ${CMAKE_SOURCE_DIR}/vendor/ImGuizmo
    ${CMAKE_SOURCE_DIR}/vendor/pfd
    ${CMAKE_SOURCE_DIR}/vendor/json/include
)

target_link_libraries(engine PUBLIC
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${LUA_LIBRARIES}
    imgui
)