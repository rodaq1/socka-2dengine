#include "SoundSystem.h"
#include "SDL_mixer.h"
#include "ecs/components/SoundComponent.h"
#include "../Entity.h"

namespace Engine {
Mix_Chunk* SoundSystem::getOrLoadChunk(const std::string &path) {
  auto it = m_chunkCache.find(path);
  if (it != m_chunkCache.end()) {
    return it->second;
  }

  Mix_Chunk *chunk = Mix_LoadWAV(path.c_str());
  if (chunk) {
    m_chunkCache[path] = chunk;
  }
  return chunk;
}

void SoundSystem::playSound(SoundComponent* sc) {
    if (sc->srcPath.empty()) return;

    Mix_Chunk* chunk = getOrLoadChunk(sc->srcPath);
    if (!chunk) return;

    Mix_VolumeChunk(chunk, static_cast<int>(sc->volume * MIX_MAX_VOLUME));

    sc->channel = Mix_PlayChannel(-1, chunk, sc->loop ? 1 : 0);
}

void SoundSystem::onShutdown() {
    for (auto& pair : m_chunkCache) {
        if (pair.second) {
            Mix_FreeChunk(pair.second);
        }
    }
    m_chunkCache.clear();
    Mix_CloseAudio();
}

void SoundSystem::onUpdate(float dt) {
    for (Entity* entity : getSystemEntities()) {
        auto* sc = entity->getComponent<SoundComponent>();
        if (!sc) continue;

        if (sc->wasTriggered) {
            playSound(sc);
            sc->wasTriggered = false;
        }

        if (sc->channel != -1) {
            if (!Mix_Playing(sc->channel)) {
                sc->channel = -1;
            }
        }
    }
}

void SoundSystem::onInit() {
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        return;
    }
    Mix_AllocateChannels(32);
}

SoundSystem::SoundSystem() {
    requireComponent<SoundComponent>();
}
} // namespace Engine