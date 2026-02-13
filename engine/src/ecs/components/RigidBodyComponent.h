#pragma once

#include "../Component.h"
#include <glm/glm.hpp>
#include <memory>

namespace Engine {
    enum class BodyType { Static, Dynamic, Kinematic };

    class RigidBodyComponent : public Component {
    public:
        BodyType bodyType = BodyType::Dynamic;

        float mass = 1.0f;
        float gravityScale = 1.0f;

        glm::vec2 velocity = {0.0f, 0.0f};
        glm::vec2 acceleration = {0.0f, 0.0f};

        float linearDrag = 0.1f;

        RigidBodyComponent(BodyType type = BodyType::Dynamic) : bodyType(type) {}

        void addForce(glm::vec2 force) {
            if (bodyType == BodyType::Dynamic && mass > 0.0f) {
                acceleration += force / mass;
            }
        }
        std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<RigidBodyComponent>(*this);
        }
    };
}