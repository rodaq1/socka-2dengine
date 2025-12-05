#pragma once

#include "Component.h"
#include <typeindex>
#include <unordered_map>
#include <memory>

#include "../scene/Scene.h"

#include "core/Log.h"
#include "entt/entity/fwd.hpp"
#include "entt/entt.hpp"

namespace Engine {

    class Scene;

    class Entity {
    private:
        std::string name;

        entt::entity m_Handle = entt::null;

        Scene* m_Scene = nullptr;
    public:
        std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
    
        Entity(entt::entity handle, Scene* scene, const std::string& name = "New entity");
        ~Entity();

        Entity(const Entity&) = delete;
        Entity& operator=(const Entity&) = delete;
        Entity(Entity&&) noexcept = default;
        Entity& operator=(Entity&&) noexcept = default;

        void init();
        void update(float dt);
        void render();
        void shutdown();

        /**
         * @brief priradi component entite.
         * @tparam T typ komponentu (datovy typ ktory musi inheritovat z compoonentu)
         * @tparam Args argumenty pre constructor componentu
         * @param args argumenty passnute do constructora componentu
         * @return T* pointer na vytvoreny a priradeny component
         */
        template<typename T, typename... Args>
        T* addComponent(Args&&... args) {
            // check aby sa nepriradovali tie iste komponenty viackrat
            if (hasComponent<T>()) {
                return getComponent<T>();
            }

            // vytvorenie componentu, transfernutie ownershipu specialnemu pointru
            std::unique_ptr<T> component = std::make_unique<T>(std::forward<Args>(args)...);

            // nastavenie owner pointru tejto entite
            component->owner = this;

            // ulozenie componentu s vyuzitim id datoveho typu ako kluc
            components[std::type_index(typeid(T))] = std::move(component);

            m_Scene->checkEntitySubscriptions(this);
            Log::info("added component ");

            return static_cast<T*>(components[std::type_index(typeid(T))].get());
        }

        /**
         * @brief vyberie konkretny component z entity
         * @tparam T typ componentu
         * @return T* pointer na component, alebo nullptr ak sa nic nenaslo.
         */
        template<typename T>
        T* getComponent() const {
            auto it = components.find(std::type_index(typeid(T)));
            if (it != components.end()) {
                return static_cast<T*>(it->second.get());
            }
            return nullptr;
        }

        /**
         * @brief checkne ci entita ma component
         * @tparam T typ componentu
         * @return bool True ak component existuje, inak false 
         */
        template<typename T>
        bool hasComponent() const {
            return components.count(std::type_index(typeid(T)));
        }

        // properties
        const std::string& getName() const { return name; }
        void setName(const std::string& n_name) { name = n_name; }

        entt::entity getHandle() const { return m_Handle; }
        Scene* getScene() const { return m_Scene; }

    };
}