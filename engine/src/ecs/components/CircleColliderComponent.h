#pragma once
#ifndef CIRCLE_COLLIDER_COMPONENT_H
#define CIRCLE_COLLIDER_COMPONENT_H

#include "../Component.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <functional>
#include "BoxColliderComponent.h"

namespace Engine {


    /**
     * @brief Component defining a circular boundary for collisions.
     */
    class CircleColliderComponent : public Component {
    public:
        glm::vec2 offset = {0.0f, 0.0f};
        float radius = 16.0f;
        
        // Layer: which group this entity belongs to (Who am I?)
        uint32_t layer = CollisionLayer::Default;

        // Mask: which groups this entity can collide with (Who do I hit?)
        uint32_t mask = CollisionLayer::All;

        bool isTrigger = false;
        bool isStatic = false;

        // Callback for trigger events
        std::function<void(Entity* other)> onTriggerEnter;

        // Standard constructor for the AddComponent pattern
        CircleColliderComponent() : Component() {}
        
        // Parameterized constructor
        CircleColliderComponent(float r, glm::vec2 off = {0.0f, 0.0f}, bool trigger = false, bool stat = false) 
            : Component(), radius(r), offset(off), isTrigger(trigger), isStatic(stat) {}

        void setLayer(CollisionLayer l) { layer = l; }
        void setMask(uint32_t m) { mask = m; } 

    };

} // namespace Engine

#endif