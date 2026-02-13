#include "InputSystem.h"
#include "core/Input.h"
#include "core/Log.h"
#include "ecs/Entity.h"
#include "ecs/components/InputControllerComponent.h"
#include "ecs/components/RigidBodyComponent.h"
#include "ecs/components/VelocityComponent.h"

namespace Engine {
InputSystem::InputSystem() {
  requireComponent<InputControllerComponent>();
  Log::info("InputSystem inicializovany.");
}

void InputSystem::onUpdate(float dt) {
  for (auto entity : getSystemEntities()) {
    auto controller = entity->getComponent<InputControllerComponent>();
    if (!controller->overrideDefaultSettings) {
      if (entity->hasComponent<VelocityComponent>()) {
        auto velocity = entity->getComponent<VelocityComponent>();

        velocity->velocity.x = 0.0f;
        velocity->velocity.y = 0.0f;

        float speed = 0.0f;
        if (controller->parameters.count("MoveSpeed")) {
          speed = controller->parameters.at("MoveSpeed");
        }

        for (const auto &binding : controller->mappings) {
          if (Input::isKeyDown(binding.scancode)) {
            if (binding.actionName == "MoveUp") {
              velocity->velocity.y = speed;
            } else if (binding.actionName == "MoveDown") {
              velocity->velocity.y = -speed;
            } else if (binding.actionName == "MoveLeft") {
              velocity->velocity.x = -speed;
            } else if (binding.actionName == "MoveRight") {
              velocity->velocity.x = speed;
            } 
          }
        }

        // diagonal movement velocity calculation.
        if (velocity->velocity.x != 0.0f && velocity->velocity.y != 0.0f) {
          glm::vec2 normalizedVel = glm::normalize(velocity->velocity);
          velocity->velocity.x = normalizedVel.x * speed;
          velocity->velocity.y = normalizedVel.y * speed;
        }
      } else if (entity->hasComponent<RigidBodyComponent>()) {
        glm::vec2 dir(0.0f);

        for (const auto &binding : controller->mappings) {
          if (Input::isKeyDown(binding.scancode)) {
            if (binding.actionName == "MoveUp") {
              dir.y += 1.0f;
            } else if (binding.actionName == "MoveDown") {
              dir.y -= 1.0f;
            } else if (binding.actionName == "MoveLeft") {
              dir.x -= 1.0f;
            } else if (binding.actionName == "MoveRight") {
              dir.x += 1.0f;
            } 
          }
        }
        float force = 50000.0f;
        entity->getComponent<RigidBodyComponent>()->addForce(dir * force);
      }
    }
  }
}
} // namespace Engine