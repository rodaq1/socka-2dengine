#pragma once

#include "Component.h"
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <string>
#include <glm/glm.hpp>

#include "entt/entt.hpp"

namespace Engine {

    class Scene;
    class TransformComponent;
    class InheritanceComponent;

    class Entity {
    private:
        std::string name;
        entt::entity m_Handle = entt::null;
        Scene* m_Scene = nullptr;
        
        std::unordered_set<std::type_index> m_PendingRemoval;

        // Internal helpers to safely access components or notify scene
        TransformComponent* getTransform() const;
        void notifySceneOfComponentChange(); // Implementation in .cpp to avoid incomplete type error

    public:
        bool isRemoved = false;
        std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
    
        Entity(entt::entity handle, Scene* scene, const std::string& name = "New entity");
        ~Entity();

        // Hierarchy Management
        void setParent(Entity* newParent);
        Entity* getParent() const;
        const std::vector<Entity*>& getChildren() const;

        // Lifecycle
        void init();
        void update(float dt);
        void render();
        void shutdown();

        // World-space transform calculations
        glm::vec2 getWorldPosition() const;        
        float getWorldRotation() const;
        glm::vec2 getWorldScale() const;

        template<typename T, typename... Args>
        T* addComponent(Args&&... args) {
            if (hasComponent<T>()) {
                return getComponent<T>();
            }

            std::unique_ptr<T> component = std::make_unique<T>(std::forward<Args>(args)...);
            component->owner = this;
            
            auto typeIdx = std::type_index(typeid(T));
            components[typeIdx] = std::move(component);

            notifySceneOfComponentChange();

            return static_cast<T*>(components[typeIdx].get());
        }

        template<typename T>
        void removeComponent() {
            auto typeIdx = std::type_index(typeid(T));
            if (m_PendingRemoval.count(typeIdx)) return;

            auto it = components.find(typeIdx);
            if (it != components.end()) {
                m_PendingRemoval.insert(typeIdx);
                it->second->onShutdown(); 

                components.erase(it);
                notifySceneOfComponentChange();

                m_PendingRemoval.erase(typeIdx);
            }
        }

        template<typename T>
        T* getComponent() const {
            auto it = components.find(std::type_index(typeid(T)));
            if (it != components.end()) {
                return static_cast<T*>(it->second.get());
            }
            return nullptr;
        }

        template<typename T>
        bool hasComponent() const {
            return components.count(std::type_index(typeid(T)));
        }

        const std::string& getName() const { return name; }
        void setName(const std::string& n_name) { name = n_name; }
        entt::entity getHandle() const { return m_Handle; }
        Scene* getScene() const { return m_Scene; }

        Entity(const Entity&) = delete;
        Entity& operator=(const Entity&) = delete;
        Entity(Entity&&) noexcept = default;
        Entity& operator=(Entity&&) noexcept = default;
    };
}