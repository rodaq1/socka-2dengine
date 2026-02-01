#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

namespace Engine {

    /**
     * @brief Monitoruje subory pre zmeny a triggeruje callbacky pre instant reload
     */
    class FileWatcher {
    public:
        enum class FileStatus { CREATED, MODIFIED, ERASED };

        FileWatcher(const std::string& rootPath) : m_RootPath(rootPath) {}

        void update(const std::function<void(std::string, FileStatus)>& callback);

        void init();
    
    private:
        std::string m_RootPath;
        std::unordered_map<std::string, std::filesystem::file_time_type> m_FileMap;
    };
}