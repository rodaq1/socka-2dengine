#pragma once

#include <cstdint>

namespace Engine {
    /**
     * @brief static util class na trackovanie casu a deltaTime
     */

    class Time {
    private:
        Time() = delete;

        static uint64_t lastFrameTicks;
        static float deltaTime;
        static float runningTime; 
    public:
        static void update();
        static float getDeltaTime() { return deltaTime; }
        static float getRunningTime() { return runningTime; }
    };
}