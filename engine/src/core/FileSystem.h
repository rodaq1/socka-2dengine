#pragma once

#include <filesystem>
#include <string>
#include "Project.h"
#include "core/Log.h"

namespace Engine {
    class FileSystem {
    public:
        static inline std::filesystem::path s_ProjectRoot;

        static bool setProjectRoot(const std::filesystem::path& path) {
            if (std::filesystem::exists(path)) {
                // Root is the directory containing the .eproj file
                s_ProjectRoot = path.parent_path();
                Log::info("Project Root set to: " + s_ProjectRoot.string());
                return true;
            }
            Log::error("Failed to set project root. Path does not exist: " + path.string());
            return false;
        }

        static std::filesystem::path getAbsolutePath(const std::string& relativePath) {
            return s_ProjectRoot / relativePath;
        }

        static std::string getRelativePath(const std::filesystem::path& absolutePath) {
            return std::filesystem::relative(absolutePath, s_ProjectRoot).string();
        }
    };
}