#include "Time.h"
#include <SDL2/SDL.h>

namespace Engine {
    uint64_t Time::lastFrameTicks = 0;
    float Time::deltaTime = 0.0f;
    float Time::runningTime = 0.0f;

    void Time::update() {
        uint64_t currentTicks = SDL_GetTicks64();

        if (lastFrameTicks > 0) {
            uint64_t deltaTicks = currentTicks - lastFrameTicks;

            deltaTime = static_cast<float>(deltaTicks) / 1000.0f;
        }

        runningTime = static_cast<float>(currentTicks) / 1000.0f;
        lastFrameTicks = currentTicks;
    }
}