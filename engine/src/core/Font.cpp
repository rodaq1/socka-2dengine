#include "Font.h"
#include <SDL2/SDL.h>
#include "Log.h"

namespace Engine {

Font::Font(const std::string& path, int size)
    : m_Path(path), m_Size(size)
{
    m_Font = TTF_OpenFont(path.c_str(), size);

    if (!m_Font)
    {
        Log::info("Failed to load font " + path + " : " + TTF_GetError());
    }
}

Font::~Font() {
    if (m_Font) {
        TTF_CloseFont(m_Font);
        m_Font = nullptr;
    }
}

}