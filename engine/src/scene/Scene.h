#pragma once 

#include "../ecs/System.h"
#include "SDL_render.h"
#include "entt/entity/fwd.hpp"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <entt/entt.hpp>


namespace Engine {

    class Entity;

    class Scene {
    private:

        std::string name;
        std::vector<std::unique_ptr<Entity>> entities;


        std::unique_ptr<entt::registry> m_Registry;

        std::unordered_map<std::type_index, std::unique_ptr<System>> m_Systems;

        SDL_Rect m_Camera;

        static SDL_Renderer* m_Renderer;

        std::unordered_map<entt::entity, std::unique_ptr<Entity>> m_EntityWrappers;

    public:
        Scene(const std::string& name = "Untitled scene");
        ~Scene();
    
        void checkEntitySubscriptions(Entity* entity);
        void checkAllEntitySubscriptions(System* system);

        void init();
        void update(float dt);
        void render();
        void shutdown();

        Entity* createEntity(const std::string& name = "New entity");
        void destroyEntity(Entity* entity);

        template <typename TSystem>
        TSystem* addSystem() {
            if (hasSystem<TSystem>()) {
                return getSystem<TSystem>();
            }
            
            TSystem* newSystem = new TSystem();
            m_Systems[std::type_index(typeid(TSystem))] = std::unique_ptr<TSystem>(newSystem);
            
            checkAllEntitySubscriptions(newSystem);

            return newSystem;
        }

        template <typename TSystem>
        TSystem* getSystem() const {
            auto it = m_Systems.find(std::type_index(typeid(TSystem)));
            if (it != m_Systems.end()) {
                return static_cast<TSystem*>(it->second.get());
            }
            return nullptr;
        }

        template <typename TSystem>
        bool hasSystem() const {
            return m_Systems.count(std::type_index(typeid(TSystem)));
        }

        template <typename TComponent>
        void onComponentAdded(Entity* entity) {
            checkEntitySubscriptions(entity);
        }

        const std::string& getName() const { return name; }
        static void setRenderer(SDL_Renderer* renderer);
    };
}