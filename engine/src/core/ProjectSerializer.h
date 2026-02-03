#pragma once
#include "core/Project.h"
#include "scene/Scene.h"
#include "core/AssetManager.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <memory>

namespace Engine {

namespace fs = std::filesystem;
using json = nlohmann::json;

class ProjectSerializer {
public:
    // Save/load project file
    static void saveProjectFile(Project* project, const fs::path& filePath);
    static bool loadProjectFile(Project* project, const fs::path& filePath);

    // Save/load scenes relative to project root
    static void saveScene(Scene* scene, const fs::path& filePath);
    static bool loadScene(std::unique_ptr<Scene>& scenePtr, SDL_Renderer* renderer,
                          const fs::path& filePath, AssetManager* assetManager);

private:
    static json serializeEntity(Entity* entity);
    static void deserializeEntity(Scene* scene, const json& j, AssetManager* assetManager);

    // Helper to find assets inside project root
        static std::string findAssetPath(const fs::path &root,
                                     const std::string &fileName,
                                     int depth = 0,
                                     int maxDepth = 5);
};

} // namespace Engine
