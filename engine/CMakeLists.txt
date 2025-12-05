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
)

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor/glm
    ${CMAKE_SOURCE_DIR}/vendor/entt/src/
)

target_link_libraries(engine PUBLIC
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
)