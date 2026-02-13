#pragma once 

#include "ecs/Component.h"
#include <string>
#include <SDL_mixer.h>
#include <memory>

namespace Engine {
    class SoundComponent : public Component {
    public:
        std::string srcPath;
        float volume = 1.0f;
        bool loop = false;
        bool playOnStart = false;

        int channel = -1;
        bool isPlaying = false;
        bool wasTriggered;

        void play() {
            wasTriggered = true;
        }
        void stop() {
            wasTriggered = false;
            if (channel != -1) {
                Mix_HaltChannel(channel);
                channel = -1;
                isPlaying = false;
            }
        }
        std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<SoundComponent>(*this);
        }
    };
}