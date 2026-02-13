#pragma once

#include "../System.h" // Includes the base System class
#include "SDL_render.h"
#include "core/Camera.h"
#include <SDL2/SDL.h>


namespace Engine {

    // Forward declarations
    class Entity;
    class TransformComponent;
    class SpriteComponent;

    /**
     * @brief System responsible for rendering entities with a TransformComponent and SpriteComponent.
     */
    class RendererSystem : public System {
    public:
        /**
         * @brief Constructs the RenderSystem with the active SDL_Renderer.
         * @param renderer The main SDL renderer pointer.
         */
        RendererSystem();
        ~RendererSystem() override = default;

        void update(SDL_Renderer* renderer, const Camera& camera, float targetWidth, float targetHeight, float dt);


    private:
        SDL_Renderer* m_Renderer = nullptr;
        // The list of entities this system manages (those with required components)
        std::vector<Entity*> m_RenderableEntities; 
    };

} // namespace Engine