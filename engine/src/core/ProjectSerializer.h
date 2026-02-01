#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>
#include "core/AssetManager.h"
#include "core/Project.h"
#include "scene/Scene.h"

namespace Engine {

    class ProjectSerializer {
    public:
        static void saveProjectFile(Project* data, const std::string& filepath);
        static bool loadProjectFile(Project* data, const std::string& filepath);

        static void saveScene(Scene* scene, const std::string& filepath);
        static bool loadScene(std::unique_ptr<Scene>& scenePtr, SDL_Renderer* renderer, const std::string& filepath, AssetManager* assetManager);

    private:
        static nlohmann::json serializeEntity(Entity* entity);
        static void deserializeEntity(Scene* scene, const nlohmann::json& entityData, AssetManager* assetManager);
    };

}