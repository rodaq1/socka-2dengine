#include "FontManager.h"
#include "Font.h"
#include <iostream>
#include "Project.h"
#include "Log.h"
namespace Engine {

std::unordered_map<std::string, std::shared_ptr<Font>> FontManager::s_Cache;

void FontManager::Init() {
    // Nothing to do for now
}

void FontManager::Shutdown() {
    s_Cache.clear();
}

std::shared_ptr<Font> FontManager::LoadFont(const std::string& path, int size, Project* project) {
    // Construct the absolute path
    std::filesystem::path fullPath = project->getPath() / "assets" / path;
    std::string realPath = fullPath.string();
    
    
    if (!std::filesystem::exists(fullPath)) {
        return nullptr;
    }

    std::string key = realPath + "#" + std::to_string(size);

    // 2. Check Cache
    auto it = s_Cache.find(key);
    if (it != s_Cache.end()) {
        return it->second;
    }

    std::shared_ptr<Font> font = std::make_shared<Font>(realPath, size);

    if (!font->isValid()) {
        return nullptr;
    }

    s_Cache[key] = font;
    return font;
}

}

