#pragma once

#include "../System.h"
#include <SDL2/SDL.h>
#include <unordered_map>
#include "core/Project.h"

namespace Engine {

class Camera;
class Entity;

class TextSystem : public System {
public:
    TextSystem();
    ~TextSystem() override;

    void update(SDL_Renderer* renderer,
                const Camera& camera,
                float targetWidth,
                float targetHeight,
                Project* project);

private:
    struct TextRenderData {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    std::unordered_map<Entity*, TextRenderData> m_TextData;
};

}