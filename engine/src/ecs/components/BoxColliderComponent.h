#pragma once

#include "../Component.h"
#include "glm/ext/vector_float2.hpp"
#include <cstdint>
#include <glm/glm.hpp>
#include <sys/types.h>

namespace Engine {

    /**
     * @brief Collision layers definovane bitflagmi
     */
    enum CollisionLayer : uint32_t {
        None = 0,
        Default = 1 << 0,
        Player = 1 << 1,
        Enemy = 1 << 2, 
        Obstacle = 1 << 3,
        Projectile = 1 << 4,
        Trigger = 1 << 5,
        All = 0xFFFFFFFF
    };

    /**
     * @brief Component definujuci bounding-box (AABB) pre kolizie.
     */
    class BoxColliderComponent : public Component {
    public:
        glm::vec2 size;
        glm::vec2 offset;
        float rotation;

        // Layer: pod ktoru skupinu patri entita (who am i?)
        uint32_t layer = CollisionLayer::Default;

        // mask: s ktorymi skupinami tato entita moze collidovat. Defaultne moze collidovat so vsetkym. (who hits me?)
        uint32_t mask = CollisionLayer::All;

        bool isTrigger;
        std::function<void(Entity* other)> onTriggerEnter;
        
        bool isStatic;

        void setLayer(CollisionLayer l) { layer = l; }
        void setMask(uint32_t m) { mask = m; } 

        BoxColliderComponent(glm::vec2 size = {32.0f, 32.0f}, glm::vec2 offset = {0.0f, 0.0f}, float rotation = 0.0f, bool isTrigger = false, bool isStatic = false)
            : size(size), offset(offset), rotation(rotation), isTrigger(isTrigger), isStatic(isStatic) {}
    };
}