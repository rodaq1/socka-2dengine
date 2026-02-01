#include "FileWatcher.h"
#include <filesystem>

namespace Engine {

    void FileWatcher::init() {
        if (!std::filesystem::exists(m_RootPath)) return;

        for (auto& file : std::filesystem::recursive_directory_iterator(m_RootPath)) {
            m_FileMap[file.path().string()] = std::filesystem::last_write_time(file);
        }
    }

    void FileWatcher::update(const std::function<void(std::string, FileStatus)>& callback) {
        for (auto& file : std::filesystem::recursive_directory_iterator(m_RootPath)) {
            auto path = file.path().string();
            auto currentWriteTime = std::filesystem::last_write_time(file);

            if (m_FileMap.find(path) == m_FileMap.end()) {
                m_FileMap[path] = currentWriteTime;
                callback(path, FileStatus::CREATED);
            } else if (m_FileMap[path] != currentWriteTime) {
                m_FileMap[path] = currentWriteTime;
                callback(path, FileStatus::MODIFIED);
            }
        }

        auto it = m_FileMap.begin();
        while (it != m_FileMap.end()) {
            if (!std::filesystem::exists(it->first)) {
                callback(it->first, FileStatus::ERASED);
                it = m_FileMap.erase(it);
            } else {
                it++;
            }
        }
    }
}