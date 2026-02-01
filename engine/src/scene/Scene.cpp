#include "Scene.h"
#include "../core/Log.h"
#include "../ecs/Entity.h"
#include "../ecs/System.h"

// System Includes
#include "ecs/systems/RendererSystem.h"
#include "ecs/systems/CameraSystem.h"
#include "ecs/systems/CollisionSystem.h"
#include "ecs/systems/InputSystem.h"
#include "ecs/systems/MovementSystem.h"
#include "ecs/systems/PhysicsSystem.h"
#include "ecs/systems/ScriptSystem.h"
#include "ecs/systems/SoundSystem.h"

namespace Engine {

SDL_Renderer* Scene::m_Renderer = nullptr;

Scene::Scene(const std::string& name) 
    : name(name), m_Registry(std::make_unique<entt::registry>()) {
    
    addSystem<RendererSystem>();
    addSystem<InputSystem>();
    addSystem<MovementSystem>();
    addSystem<CollisionSystem>();
    addSystem<PhysicsSystem>();
    addSystem<CameraSystem>();
    addSystem<ScriptSystem>();
    addSystem<SoundSystem>();
}

Scene::~Scene() {
    Log::info("Scene erased: " + name);
    m_Systems.clear();
    m_EntityWrappers.clear();
}

Entity* Scene::createEntity(const std::string& eName) {
    entt::entity handle = m_Registry->create();
    auto entity = std::make_unique<Entity>(handle, this, eName);
    
    Entity* raw_ptr = entity.get();
    m_EntityWrappers[handle] = std::move(entity);

    m_Registry->emplace<Entity*>(handle, raw_ptr);

    Log::info("Entity '" + eName + "' created with handle: " + std::to_string(static_cast<std::uint32_t>(handle)));
    return raw_ptr;
}

void Scene::destroyEntity(Entity* entity) {
    if (!entity || entity->isRemoved) return;

    entity->isRemoved = true;
    entt::entity handle = entity->getHandle();

    for (auto& [_, system] : m_Systems) {
        system->removeEntity(entity);
    }

    entity->shutdown();

    if (m_Registry && handle != entt::null) {
        m_Registry->destroy(handle);
    }

    m_EntityWrappers.erase(handle);
}

void Scene::checkEntitySubscriptions(Entity* entity) {
    for (auto const& [type, system] : m_Systems) {
        bool matchesSignature = true;
        
        for (auto const& requiredTypeInfo : system->getComponentSignature()) {
            if (entity->components.find(std::type_index(*requiredTypeInfo)) == entity->components.end()) {
                matchesSignature = false;
                break;
            }
        }

        if (matchesSignature) {
            system->addEntity(entity);
        } else {
            system->removeEntity(entity);
        }
    }
}

void Scene::checkAllEntitySubscriptions(System* system) {
    for (auto const& [handle, entityPtr] : m_EntityWrappers) {
        bool matchesSignature = true;
        for (auto const& requiredTypeInfo : system->getComponentSignature()) {
            if (entityPtr->components.find(std::type_index(*requiredTypeInfo)) == entityPtr->components.end()) {
                matchesSignature = false;
                break;
            }
        }
        if (matchesSignature) {
            system->addEntity(entityPtr.get());
        }
    }
}

void Scene::setRenderer(SDL_Renderer* renderer) {
    m_Renderer = renderer;
}

void Scene::init() {
    Log::info(name + " init starting");
    for (auto const& [type, system] : m_Systems) {
        system->onInit();
    }
    
    for (auto const& [handle, entity] : m_EntityWrappers) {
        entity->init();
    }
    Log::info(name + " init finished");
}

void Scene::update(float dt) {
    // Update the main scene camera based on its entity's transform
    if (auto* camSystem = getSystem<CameraSystem>()) {
        camSystem->updateCamera(&m_SceneCamera);
    }

    // Update all other systems
    for (auto const& [type, system] : m_Systems) {
        system->onUpdate(dt);
    }
    
    // Update all entities (and their components)
    for (auto const& [handle, entity] : m_EntityWrappers) {
        entity->update(dt);
    }
}

void Scene::render(SDL_Renderer* renderer, Camera& camera, float renderW, float renderH) {
    if (auto* renderSys = getSystem<RendererSystem>()) {
        renderSys->update(renderer, camera, renderW, renderH);
    }
}

void Scene::shutdown() {
    for (auto const& [type, system] : m_Systems) {
        system->onShutdown();
    }

    for (auto const& [handle, entity] : m_EntityWrappers) {
        entity->shutdown();
    }

    m_Registry->clear();
    m_EntityWrappers.clear();
}

} // namespace Engine
