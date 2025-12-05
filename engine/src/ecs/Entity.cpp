#include "Entity.h"
#include "../core/Log.h"

namespace Engine {
    Entity::Entity(entt::entity handle, Scene* scene, const std::string& name) : name(name), m_Handle(handle), m_Scene(scene) {
        Log::info("New entity created: " + name);
    }

    Entity::~Entity() {
        Log::info("Entity destroyed: " + name);
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
}