#include "Entity.h"
#include "core/Log.h"
#include "ecs/components/InheritanceComponent.h"
#include "ecs/components/TransformComponent.h"
#include "../scene/Scene.h" 
#include <algorithm>
#include <cmath>

namespace Engine {

Entity::Entity(entt::entity handle, Scene *scene, const std::string &name)
    : name(name), m_Handle(handle), m_Scene(scene) {
    Log::info("New entity created: " + name);
    this->addComponent<TransformComponent>();
}

Entity::~Entity() { 
    Log::info("Entity destroyed: " + name); 
}

void Entity::notifySceneOfComponentChange() {
    if (m_Scene) {
        m_Scene->checkEntitySubscriptions(this);
    }
}

TransformComponent* Entity::getTransform() const {
    return getComponent<TransformComponent>();
}

Entity* Entity::getParent() const {
    auto* inh = getComponent<InheritanceComponent>();
    return inh ? inh->parent : nullptr;
}

const std::vector<Entity*>& Entity::getChildren() const {
    static std::vector<Entity*> empty;
    auto* inh = getComponent<InheritanceComponent>();
    return inh ? inh->children : empty;
}

void Entity::setParent(Entity* newParent) {
    if (newParent == this) {
        Log::error("Entity cannot be its own parent");
        return;
    }

    auto* inheritance = getComponent<InheritanceComponent>();
    if (!inheritance) {
        inheritance = addComponent<InheritanceComponent>();
    }

    if (inheritance->parent) {
        auto* oldParentInh = inheritance->parent->getComponent<InheritanceComponent>();
        if (oldParentInh) {
            auto& children = oldParentInh->children;
            children.erase(std::remove(children.begin(), children.end(), this), children.end());
        }
    }

    inheritance->parent = newParent;

    if (newParent) {
        auto* newParentInh = newParent->getComponent<InheritanceComponent>();
        if (!newParentInh) {
            newParentInh = newParent->addComponent<InheritanceComponent>();
        }
        newParentInh->children.push_back(this);
    }
}

glm::vec2 Entity::getWorldPosition() const {
    TransformComponent* transform = getTransform();
    if (!transform) return {0.0f, 0.0f};

    Entity* parent = getParent();
    if (parent) {
        glm::vec2 parentPos = parent->getWorldPosition();
        float parentRot = parent->getWorldRotation();
        glm::vec2 parentScale = parent->getWorldScale();

        float rad = glm::radians(parentRot);
        float cosR = std::cos(rad);
        float sinR = std::sin(rad);

        glm::vec2 rotatedLocalPos;
        rotatedLocalPos.x = (transform->position.x * cosR) - (transform->position.y * sinR);
        rotatedLocalPos.y = (transform->position.x * sinR) + (transform->position.y * cosR);

        return parentPos + (rotatedLocalPos * parentScale);
    }
    
    return transform->position;
}

float Entity::getWorldRotation() const {
    TransformComponent* transform = getTransform();
    if (!transform) return 0.0f;

    Entity* parent = getParent();
    if (parent) {
        return parent->getWorldRotation() + transform->rotation;
    }
    return transform->rotation;
}

glm::vec2 Entity::getWorldScale() const {
    TransformComponent* transform = getTransform();
    if (!transform) return {1.0f, 1.0f};

    Entity* parent = getParent();
    if (parent) {
        return parent->getWorldScale() * transform->scale;
    }
    return transform->scale;
}

void Entity::init() {
    for (auto const& [type, component] : components) {
        component->onInit();
    }
}

void Entity::update(float dt) {
    for (auto const& [type, component] : components) {
        component->onUpdate(dt);
    }
}

void Entity::render() {
    for (auto const& [type, component] : components) {
        component->onRender();
    }
}

void Entity::shutdown() {
    for (auto const& [type, component] : components) {
        component->onShutdown();
    }
    components.clear();
}

} // namespace Engine