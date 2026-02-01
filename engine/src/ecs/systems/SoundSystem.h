#pragma once

#include "SDL_mixer.h"
#include "ecs/System.h"
#include "ecs/components/SoundComponent.h"
#include <string>
#include <unordered_map>
namespace Engine {

    class SoundSystem : public System {
    public:
        SoundSystem();
        ~SoundSystem() override = default;

        void onInit() override;
        void onUpdate(float dt) override;
        void onShutdown() override;
    
    private:
        std::unordered_map<std::string, Mix_Chunk*> m_chunkCache;
        
        void playSound(SoundComponent* sc);
        Mix_Chunk* getOrLoadChunk(const std::string& path);
    };
}