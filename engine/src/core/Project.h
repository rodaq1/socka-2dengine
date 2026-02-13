#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace Engine {

    class Scene; 
    
    struct ProjectConfig {
        std::string name = "New Project";
        std::string assetDirectory = "assets";
        std::string startScenePath;
        std::string engineVersion = "1.0.0";
        int width = 1280;
        int height = 720;
    };

    struct ProjectRuntimeState {
        std::string lastActiveScene;
    };

    class Project {
    public:
        static const std::string Extension;

        ProjectConfig config;
        ProjectRuntimeState runtime;
        std::filesystem::path projectPath;

        std::string m_PendingSceneName;
        bool m_SceneLoadRequested = false;

        Scene* activeScene;
        
        bool isLoaded = false; 

        Project() = default;

        std::string getProjectName() const { return config.name; }
        std::filesystem::path getAssetPath() const { return projectPath / config.assetDirectory; }
        std::filesystem::path getPath() const { return projectPath; }

        Scene* getActiveScene() {
            return activeScene;
        }
        void setActiveScene(Scene* scene) {
            activeScene = scene;
        }

        void requestSceneLoad(const std::string& name) {
            m_PendingSceneName = name;
            m_SceneLoadRequested = true;
        }

        std::string getProjectFileName() const {
            return config.name + ".eproj";
        }

        std::string getProjectFilePath() const {
            return (projectPath / getProjectFileName()).string();
        }

        ProjectConfig& getConfig() { return config; }
        ProjectRuntimeState& getRuntime() { return runtime; }

        /**
         * @brief Updates the config. 
         * IMPORTANT: Only call this from the Serializer after successfully reading from disk.
         */
        void setConfig(const ProjectConfig& conf) {
            config = conf;
            isLoaded = true; // Mark as valid/hydrated
        }

        void setName(const std::string& name) {
            config.name = name;
        }

        void setPath(const std::string& path) {
            projectPath = path;
        }
    };

    inline const std::string Project::Extension = ".eproj";
}