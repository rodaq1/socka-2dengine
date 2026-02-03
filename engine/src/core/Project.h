#pragma once

#include <string>
#include <vector>
#include <filesystem>


namespace Engine {
    struct ProjectConfig {
        std::string name = "New Project";
        std::string assetDirectory = "assets";
        std::string startScenePath = "scenes/main.json";
        std::string engineVersion = "1.0.0";
        std::string lastActiveScene = "main.json";
        int width = 1280;
        int height = 720;
    };

    class Project {
    public:
        static const std::string Extension;

        ProjectConfig config;
        std::filesystem::path projectPath;

        Project() = default;

        std::string getProjectName() const {return config.name;}
        std::filesystem::path getAssetPath() const {return projectPath / config.assetDirectory;}
        std::filesystem::path getPath() const {return projectPath;}
        std::string getProjectFileName() const {
            return config.name + ".eproj";
        }

        std::string getProjectFilePath() const {
            std::filesystem::path fullPath = projectPath / getProjectFileName();
            return fullPath.string();
        }
        ProjectConfig& getConfig() { return config; }


        void setName(const std::string& name) {
            config.name = name;
        }

        void setConfig(ProjectConfig conf) {
            conf = config;
        }

        /**
         * @brief Sets the absolute file path to the project file (.eproj).
         * @param path The filesystem path as a string.
         */
        void setPath(const std::string& path) {
            projectPath = path;
        }




    };

    inline const std::string Project::Extension = ".eproj";
}