#pragma once
#include <string>
#include <memory>
#include <memory>
#include <SDL2/SDL.h>
#include "../Component.h"

namespace Engine {

class Font;

class TextComponent : public Component {
public:
    std::string text = "Text";
    std::string fontPath;
    int fontSize = 24;

    SDL_Color color {255,255,255,255};

    bool isFixed = false;
    int zIndex = 0;

    std::shared_ptr<Font> font = nullptr;

    bool dirty = true;
    int lastFontSize = 0;

    TextComponent()
    : text("Text"), fontPath("assets/fonts/Default.ttf"), fontSize(24), color{255,255,255,255}, isFixed(false), zIndex(0), font(nullptr), dirty(true) {}

    std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<TextComponent>(*this);
        }
};

}