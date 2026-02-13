#pragma once
#include <memory>

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

        bool isPrimary = false;
        std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<CameraComponent>(*this);
        }
    };

} 
