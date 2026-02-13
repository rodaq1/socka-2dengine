#pragma once

#include "../Component.h"
#include <map>
#include <string>
#include <memory>

namespace Engine
{

    /**
     * @brief metadata jedneho animation statu (napr. walk, idle...)
     * @param row is a row index in the spritesheet
     * @param frameCount is the total frames in a sequence
     * @param speed is a delay between frames in milliseconds
     */
    struct Animation
    {
        int row;
        int frameCount;
        float speed;
    };

    /**
     * @brief Komponent co umoznuje frame-based animacie pre sprite
     */
    class AnimationComponent : public Component
    {
    public:
        int currentFrame = 0;
        float startTime = 0.0f;

        std::string currentAnimationName;
        std::map<std::string, Animation> animations;

        bool isLooping = true;
        bool isPlaying = true;

        float timer = 0.0f;

        AnimationComponent() = default;

        /**
         * @brief definicia animation statu
         *
         * @param name kluc
         * @param row riadok textury
         * @param frames pocet snimkov v riadku
         * @param speed rychlost (lower -> faster)
         */
        void addAnimation(const std::string &name, int row, int frames, float speed)
        {
            animations[name] = {row, frames, speed};
            if (currentAnimationName.empty())
            {
                currentAnimationName = name;
            }
        }

        void play(const std::string &name)
        {
            if (currentAnimationName != name)
            {
                currentAnimationName = name;
                currentFrame = 0;
                timer = 0.0f;
            }
            isPlaying = true;
        }

        void stop()
        {
            isPlaying = false;
        }

        std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<AnimationComponent>(*this);
        }
    };
}