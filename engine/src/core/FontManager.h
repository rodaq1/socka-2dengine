#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "Font.h"

namespace Engine {

class Project; // forward declare

class FontManager {
public:
    static void Init();
    static void Shutdown();

    // Loads a font or returns it from the cache if already loaded
    static std::shared_ptr<Font> LoadFont(const std::string& path, int size, Project* project);

private:
    // key = path + size to differentiate fonts with same path but different sizes
    static std::unordered_map<std::string, std::shared_ptr<Font>> s_Cache;
};

}
