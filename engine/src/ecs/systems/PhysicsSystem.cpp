#include "PhysicsSystem.h"
#include "ecs/Entity.h"
#include "ecs/components/RigidBodyComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/VelocityComponent.h"

namespace Engine {
    PhysicsSystem::PhysicsSystem() {
        requireComponent<TransformComponent>();
    }

    void PhysicsSystem::onUpdate(float dt) {
        const float gravityY = 9.81f * 100.0f; 
        glm::vec2 gravity(0.0f, gravityY);

        for (auto entity : getSystemEntities()) {
            auto transform = entity->getComponent<TransformComponent>();
            
            if (entity->hasComponent<RigidBodyComponent>()) {
                auto rb = entity->getComponent<RigidBodyComponent>();

                if (rb->bodyType == BodyType::Static) {
                    rb->velocity = glm::vec2(0.0f);
                    rb->acceleration = glm::vec2(0.0f);
                    continue;
                }

                if (rb->bodyType == BodyType::Dynamic) {
                    if (rb->gravityScale != 0.0f) {
                        rb->acceleration += gravity * rb->gravityScale;
                    }
            
                    rb->velocity += rb->acceleration * dt;
                    
                    rb->velocity *= (1.0f - rb->linearDrag * dt); 
                    
                    transform->position += rb->velocity * dt;
                    
                    rb->acceleration = glm::vec2(0.0f);
                }
            
                if (rb->bodyType == BodyType::Kinematic) {
                    transform->position += rb->velocity * dt;
                } 
            } 
            else if (entity->hasComponent<VelocityComponent>()) {
                auto vel = entity->getComponent<VelocityComponent>();
                transform->position += vel->velocity * dt;
            }
        }
    }
}