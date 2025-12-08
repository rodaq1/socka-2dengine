#include "../core/Log.h"
#include "../ecs/Entity.h"


#include "../ecs/systems/RendererSystem.h"

#include <SDL2/SDL.h>
#include <memory>
#include <string>

#include "SDL_render.h"
#include "ecs/systems/MovementSystem.h"
#include "entt/entity/fwd.hpp"
#include "entt/entt.hpp"


namespace Engine {

    SDL_Renderer* Scene::m_Renderer = nullptr;

    Scene::Scene(const std::string& name) : name(name), m_Registry(std::make_unique<entt::registry>()) {
        Log::info("Scene created: " + name);

        addSystem<RendererSystem>();
        addSystem<MovementSystem>();

        m_Camera.x = 0;
        m_Camera.y = 0;
        m_Camera.w = 1280;
        m_Camera.h = 720;
    }

    Scene::~Scene() {
        Log::info("Scene erased: " + name);
        m_Systems.clear();
    }

    Entity* Scene::createEntity(const std::string& eName) {
        entt::entity handle = m_Registry->create();

        std::unique_ptr<Entity> entity = std::make_unique<Entity>(handle, this, eName);

        Entity* raw_ptr = entity.get();
        m_EntityWrappers[handle] = std::move(entity);

        m_Registry->emplace<Entity*>(handle, raw_ptr);

        Log::info("Entity '" + eName + "' vytvorena s handle: " + std::to_string(static_cast<std::uint32_t>(handle)));
        return raw_ptr;
    }

    void Scene::destroyEntity(Entity* entity) {
        if (!entity) return;

        for (auto const& [type, system] : m_Systems) {
            system->removeEntity(entity);
        }

        entity->shutdown();
        m_Registry->destroy(entity->getHandle());
        Log::info("Entity znicena: " + entity->getName());
        delete entity;
    }

    void Scene::checkEntitySubscriptions(Entity* entity) {
        for (auto const& [type, system] : m_Systems) {
            bool matchesSignature = true;
            
            for (auto const& requiredTypeInfo : system->getComponentSignature()) {
                bool hasComponent = false;
                for (auto const& [componentType, componentPtr] : entity->components) {
                    if (componentType == std::type_index(*requiredTypeInfo)) {
                        hasComponent = true;
                        break;
                    }
                }

                if (!hasComponent) {
                    matchesSignature = false;
                    break;
                }
            }

            if (matchesSignature) {
                Log::info("adding entity " + entity->getName());
                system->addEntity(entity);
            } else {
                Log::warn("removing entity " + entity->getName());
                system->removeEntity(entity);
            }
        }
    }
    
    void Scene::checkAllEntitySubscriptions(System* system) {
        m_Registry->view<Entity>().each([this, system](auto entity_handle, Entity& entity) {
            bool matchesSignature = true;
            for (auto const& requiredTypeInfo : system->getComponentSignature()) {
                bool hasComponent = false;
                for (auto const& [componentType, componentPtr] : entity.components) {
                    if (componentType == std::type_index(*requiredTypeInfo)) {
                        hasComponent = true;
                        break;
                    }
                }

                if (!hasComponent) {
                    matchesSignature = false;
                    break;
                }
            }

            if (matchesSignature) {
                system->addEntity(&entity);
            }
        });
    }

    void Scene::setRenderer(SDL_Renderer* renderer) {
        m_Renderer = renderer;
    }

    void Scene::init() {
        Log::info(name + " init starting");
        for (auto const& [type, system] : m_Systems) {
            system->onInit();
        }
        m_Registry->view<Entity*>().each([this](Entity*& entity_ptr) {
            Entity& entity = *entity_ptr;
            entity.init();
            Log::info("Entity " + entity.getName() + " inicializovana.");
        });
        Log::info(name + " init finished");
    }

    void Scene::update(float dt) {
        for (auto const& [type, system] : m_Systems) {
            system->onUpdate(dt);
        }
        m_Registry->view<Entity*>().each([this, dt](Entity*& entity_ptr) {
            Entity& entity = *entity_ptr;
            entity.update(dt);
        });
    }

    void Scene::render() {
        if (hasSystem<RendererSystem>()) {
            getSystem<RendererSystem>()->update(m_Renderer, m_Camera);
        }
        m_Registry->view<Entity*>().each([this](Entity*& entity_ptr) {
            Entity& entity = *entity_ptr;
            entity.render();
        });
       
    }

    void Scene::shutdown() {
        for (auto const& [type, system] : m_Systems) {
            system->onShutdown();
        }

        m_Registry->view<Entity*>().each([this](Entity*& entity_ptr) {
            Entity& entity = *entity_ptr;
            entity.shutdown();
            Log::info("Entity " + entity.getName() + " inicializovana.");
        });

        m_Registry->clear();
    }
}