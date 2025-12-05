#include "RendererSystem.h"
#include "../components/TransformComponent.h"
#include "../components/SpriteComponent.h"
#include "../Entity.h"
#include "SDL_rect.h"
#include "SDL_render.h"
#include "../../core/Log.h"

namespace Engine {

    /**
     * @brief Check ci sa entita nachadza vo viewporte kamery
     */
    static bool isEntityVisible(const SDL_Rect& entityRect, const SDL_Rect& camera) {
        return (entityRect.x + entityRect.w > camera.x &&
                entityRect.y + entityRect.h > camera.y &&
                entityRect.x < camera.x + camera.w &&
                entityRect.y < camera.y + camera.h);
    }

    RendererSystem::RendererSystem() {
        requireComponent<TransformComponent>();
        requireComponent<SpriteComponent>();
    }

    void RendererSystem::update(SDL_Renderer* renderer, const SDL_Rect& camera) {
        for (auto entity : getSystemEntities()) {
            auto transform = entity->getComponent<TransformComponent>();
            auto sprite = entity->getComponent<SpriteComponent>();

            if (!sprite->texture) {
                Log::warn("Entity " + entity->getName() + " has null texture.");
                continue;
            }

            SDL_Rect destRect = {
              static_cast<int>(transform->position.x - camera.x),
              static_cast<int>(transform->position.y - camera.y),
              static_cast<int>(sprite->sourceRect.w * transform->scale.x),
              static_cast<int>(sprite->sourceRect.h * transform->scale.y)   
            };

            SDL_Rect worldRect = {
              static_cast<int>(transform->position.x),
              static_cast<int>(transform->position.y),
              destRect.w,
              destRect.h
            };
            
            if (sprite->isFixed) {
                destRect.x = static_cast<int>(transform->position.x);
                destRect.y = static_cast<int>(transform->position.y);
            } else {
                if (!isEntityVisible(worldRect, camera)) {
                    continue; 
                }
            }

            SDL_SetTextureColorMod(sprite->texture, sprite->color.r, sprite->color.g, sprite->color.b);
            SDL_SetTextureAlphaMod(sprite->texture, sprite->color.a);
            
            SDL_RenderCopyEx(
                renderer, 
                sprite->texture, 
                &sprite->sourceRect, 
                &destRect,
                transform->rotation, 
                NULL,               
                SDL_FLIP_NONE       
            );
            
           // Log::info("RendererSystem drew entity: " + entity->getName() + " to position (" + std::to_string(destRect.x) + ", " + std::to_string(destRect.y) + ")");
        }
    }
}