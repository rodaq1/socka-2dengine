#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include "SDL_render.h"
#include "entt/entt.hpp"
#include "core/Camera.h"

namespace Engine {

class Entity;
class System;


enum class BackgroundType {
    Solid, Gradient, Image
};

struct BackgroundSettings{
    BackgroundType type = BackgroundType::Solid;
    
    glm::vec4 color1 = {30/255.0f, 30/255.0f, 30/255.0f, 1.0f};
    glm::vec4 color2 = {10/255.0f, 10/255.0f, 10/255.0f, 1.0f};

    std::string assetId = "";
    bool stretch = true;
};

class Scene {

private:
    std::string name;
    std::unordered_map<entt::entity, std::unique_ptr<Entity>> m_EntityWrappers;
    
    std::unique_ptr<entt::registry> m_Registry;
    std::unordered_map<std::type_index, std::unique_ptr<System>> m_Systems;
    
    Camera m_SceneCamera;
    
    static SDL_Renderer* m_Renderer;

    BackgroundSettings m_Background;
public:
    Scene(const std::string& name = "Untitled scene");
    ~Scene();

    void init();
    void update(float dt);
    void render(SDL_Renderer* renderer, Camera& camera, float renderW, float renderH);
    void shutdown();

    Entity* createEntity(const std::string& name = "New entity");
    void destroyEntity(Entity* entity);

    void checkEntitySubscriptions(Entity* entity);
    void checkAllEntitySubscriptions(System* system);

    // Getters
    const std::string& getName() const { return name; }
    void setName(std::string newName) {name = newName;}
    static void setRenderer(SDL_Renderer* renderer);
    static SDL_Renderer* getRenderer() { return m_Renderer; }
    
    Camera* getSceneCamera() { return &m_SceneCamera; }
    
    entt::registry& getRegistry() { return *m_Registry; }

    BackgroundSettings& getBackground() { return m_Background; }
    void setBackground(const BackgroundSettings& settings) { m_Background = settings; }


    std::vector<Entity*> getEntityRawPointers() {
        std::vector<Entity*> ptrs;
        ptrs.reserve(m_EntityWrappers.size());
        for (auto const& [handle, entityPtr] : m_EntityWrappers) {
            ptrs.push_back(entityPtr.get());
        }
        return ptrs;
    }

    template <typename TSystem> 
    TSystem* addSystem() {
        if (hasSystem<TSystem>()) {
            return getSystem<TSystem>();
        }
        auto newSystem = std::make_unique<TSystem>();
        TSystem* rawPtr = newSystem.get();
        m_Systems[std::type_index(typeid(TSystem))] = std::move(newSystem);
        checkAllEntitySubscriptions(rawPtr);
        return rawPtr;
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
};

} // namespace Engine
