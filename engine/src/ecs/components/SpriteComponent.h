#pragma once

#include "../Component.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "core/Log.h"
#include <SDL2/SDL.h>
#include <memory>
#include <cmath>
#include <string>

namespace Engine {
    /**
     * @brief Komponent drziaci vizualne assety a rendering vlastnosti.
     * * Uklada SDL_Texturu ziskanu cez AssetManager na assetId
     * a definuje ktoru cast textury vykreslit.
     */
    class SpriteComponent : public Component {
    public: 
        SDL_Texture* texture = nullptr;
        std::string assetId;
        SDL_Rect sourceRect;
        bool isFixed = false; // sprite position relativna ku obrazovke ak true

        bool visible = true;
        bool flipH = false;
        bool flipV = false;
        int zIndex = 0;

        SDL_Color color = {255, 255, 255, 255};

        SpriteComponent(const std::string& id = "", SDL_Texture* tex = nullptr,
                        int srcX = 0, int srcY = 0, int srcW = 0, int srcH = 0, int z = 0)
            : assetId(id), texture(tex), zIndex(z) {
            
            this->sourceRect.x = srcX;
            this->sourceRect.y = srcY;
            this->sourceRect.w = srcW;
            this->sourceRect.h = srcH;

            if (tex && srcW == 0 && srcH == 0) {
                Uint32 format;
                int access, w, h;
                SDL_QueryTexture(tex, &format, &access, &w, &h);
                sourceRect.w = w;
                sourceRect.h = h;
                
            }
            std::string debugMsg = "Sprite Created - ID: " + id + " zIndex: " + std::to_string(z);
            Log::info(debugMsg);
        }
        std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<SpriteComponent>(*this);
        }
    };
}
