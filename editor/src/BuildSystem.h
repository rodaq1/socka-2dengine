#pragma once
#include "core/Log.h"
#include <cstdlib>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

class BuildSystem {
public:
    enum class Target { Linux, Windows };

    static fs::path FindEngineRoot(fs::path startPath) {
        fs::path current = fs::absolute(startPath);
        for (int i = 0; i < 6; ++i) {
            if (fs::exists(current / "engine" / "bin")) return current;
            if (!current.has_parent_path()) break;
            current = current.parent_path();
        }
        return startPath;
    }

    static bool Build(const std::string& projectName, fs::path projectRoot, fs::path engineRoot, Target target) {
        projectRoot = fs::absolute(projectRoot);
        engineRoot = fs::absolute(engineRoot);

        std::string targetStr = (target == Target::Windows ? "Win64" : "Linux");
        fs::path exportDir = projectRoot / "Export" / targetStr;
        
        std::string runtimeName = (target == Target::Windows) ? "Runtime.exe" : "Runtime";

        fs::path runtimeTemplate = engineRoot / "editor" / runtimeName;
        if (!fs::exists(runtimeTemplate)) {
            runtimeTemplate = engineRoot / "editor" / runtimeName;
        }

        if (!fs::exists(runtimeTemplate)) {
            Engine::Log::error("Pre-compiled runtime not found at: " + runtimeTemplate.string());
            Engine::Log::error("Please build the 'Runtime' target in your engine solution first.");
            return false;
        }

        try {
            if (fs::exists(exportDir)) fs::remove_all(exportDir);
            fs::create_directories(exportDir);

            std::string gameExeName = (target == Target::Windows) ? projectName + ".exe" : projectName;
            fs::copy_file(runtimeTemplate, exportDir / gameExeName);
            Engine::Log::info("Copied runtime binary.");

            fs::path projectFile = projectRoot / "NewProject.eproj";
            if (fs::exists(projectFile)) {
                fs::copy_file(projectFile, exportDir / "data.config", fs::copy_options::overwrite_existing);
                Engine::Log::info("Generated data.config from project file.");
            } else {
                Engine::Log::error("NewProject.eproj missing! Runtime won't know what to load.");
            }

            fs::path assetSrc = projectRoot / "assets";
            if (fs::exists(assetSrc)) {
                fs::copy(assetSrc, exportDir / "assets", fs::copy_options::recursive);
                Engine::Log::info("Assets packaged.");
            }

            fs::path sceneSrc = projectRoot / "scenes";
            if (fs::exists(sceneSrc)) {
                fs::copy(sceneSrc, exportDir / "scenes", fs::copy_options::recursive);
                Engine::Log::info("Scenes packaged.");
            }

            fs::path binFolder = runtimeTemplate.parent_path();
            for (const auto& entry : fs::directory_iterator(binFolder)) {
                if (entry.path().extension() == ".dll" || entry.path().extension() == ".so") {
                    fs::copy_file(entry.path(), exportDir / entry.path().filename());
                }
            }

            Engine::Log::info("Build successful! Output: " + exportDir.string());
            return true;

        } catch (const std::exception& e) {
            Engine::Log::error("Fast Build failed: " + std::string(e.what()));
            return false;
        }
    }
};