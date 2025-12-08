#include "MovementSystem.h"
#include "../components/TransformComponent.h"
#include "../components/VelocityComponent.h"
#include "../../core/Log.h"
#include "../Entity.h"

namespace Engine {

    MovementSystem::MovementSystem() {
        requireComponent<TransformComponent>();
        requireComponent<VelocityComponent>();
        Log::info("Movement system initialized.");
    }

    void MovementSystem::onUpdate(float dt) {
        for (auto entity : getSystemEntities()) {
            auto transform = entity->getComponent<TransformComponent>();
            auto velocity = entity->getComponent<VelocityComponent>();

            transform->position.x += velocity->velocity.x * dt;
            transform->position.y += velocity->velocity.y * dt;
        }
    }

}