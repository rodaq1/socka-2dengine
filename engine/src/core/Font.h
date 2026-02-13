#pragma once
#pragma once
#include <SDL2/SDL_ttf.h>
#include <string>

namespace Engine {

class Font {
public:
    Font(const std::string& path, int size);
    ~Font();

    TTF_Font* get() const { return m_Font; }
    const std::string& getPath() const { return m_Path; }
    int getSize() const { return m_Size; }

    bool isValid() const { return m_Font != nullptr; }

private:
    std::string m_Path;
    int m_Size = 0;
    TTF_Font* m_Font = nullptr;
};

}