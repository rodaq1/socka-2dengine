#pragma once

#include "ecs/Component.h"
#include "ecs/Entity.h"
#include "glm/ext/vector_float2.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
namespace Engine {
    class PolygonColliderComponent : public Component {
    public:
        std::vector<glm::vec2> vertices;

        glm::vec2 offset = {0.0f, 0.0f};
        float rotation = 0.0f;

        uint32_t layer = 1;
        uint32_t mask = 0xFFFFFFFF;

        bool isTrigger = false;
        bool isStatic = false;

        std::function<void(Entity* other)> onTriggerEnter;

        PolygonColliderComponent() {
            vertices = {
                {0.0f, -16.0f}, {16.0f, 16.0f}, {-16.0f, 16.0f}
            };
        }
        std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<PolygonColliderComponent>(*this);
        }

        PolygonColliderComponent(const std::vector<glm::vec2>& vertices, glm::vec2 offset = {0.0f, 0.0f}, bool isTrigger = false)
            : vertices(vertices), offset(offset), isTrigger(isTrigger) {}

        void addVertex(const glm::vec2& p) {vertices.push_back(p);}
        void clearVertices(){ vertices.clear(); }

    };
}