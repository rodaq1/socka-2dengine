#pragma once

#include "../Component.h"

namespace Engine {

    /**
     * @brief A tag component to identify an entity as a camera.
     * The camera's properties are stored and managed by the main Camera class,
     * while this component links the camera's behavior to an entity's transform.
     */
    class CameraComponent : public Component {
    public:
        CameraComponent() = default;

        // If true, this will be considered the main camera for the scene.
        bool isPrimary = true;
    };

} // namespace Engine
