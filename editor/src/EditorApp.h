#pragma once

#include "core/AssetManager.h"
#include <memory>
#include <SDL2/SDL.h>

namespace Engine {
    class Scene;
}

class EditorApp{
public:
    EditorApp();
    ~EditorApp();

    void run();

private: 
    void initialize();
    void shutdown();

    void processEvents();
    void update();
    void render();

    bool m_isRunning;

    SDL_Window* m_Window;
    SDL_Renderer* m_Renderer;
    SDL_GLContext m_GLContext;

    std::unique_ptr<Engine::Scene> currentScene; 

    std::unique_ptr<Engine::AssetManager> m_AssetManager;
};