#pragma once

#include "../Component.h"
#include "glm/ext/vector_float2.hpp"
#include <memory>
#include <glm/glm.hpp>

namespace Engine {
    /**
     * @brief Component drziaci data o vektore pohybu entity.
     */
    class VelocityComponent : public Component {
    public: 
        glm::vec2 velocity = {0.0f, 0.0f};

        VelocityComponent(glm::vec2 vel = {0.0f, 0.0f})
            : velocity(vel) {}
            std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<VelocityComponent>(*this);
        }
    };
}