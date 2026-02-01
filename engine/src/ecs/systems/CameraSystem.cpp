#include "CameraSystem.h"
#include "core/Camera.h"
#include "ecs/components/CameraComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/Entity.h"

namespace Engine {

CameraSystem::CameraSystem() {
    requireComponent<TransformComponent>();
    requireComponent<CameraComponent>();
}

void CameraSystem::updateCamera(Camera* mainCamera) {
    if (!mainCamera) {
        return;
    }

    for (auto entity : getSystemEntities()) {
        // Find the primary camera entity
        auto cameraComp = entity->getComponent<CameraComponent>();
        if (cameraComp && cameraComp->isPrimary) {
            auto transform = entity->getComponent<TransformComponent>();
            if (transform) {
                mainCamera->setPosition(transform->position);
                mainCamera->setRotation(transform->rotation);
            }
            break; // Assume only one primary camera
        }
    }
}

} // namespace Engine
