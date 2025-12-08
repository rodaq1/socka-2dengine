#include "EditorApp.h"
#include "SDL_render.h"
#include "core/Log.h"
#include "core/Time.h"
#include "core/Input.h"
#include "ecs/components/VelocityComponent.h"
#include "glm/ext/vector_float2.hpp"
#include "scene/Scene.h"
#include "ecs/Component.h"
#include "ecs/Entity.h"
#include "ecs/components/TransformComponent.h"
#include "core/AssetManager.h"
#include "ecs/components/SpriteComponent.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <memory>
#include <stdexcept>
#include <string>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

using Engine::Log;
using Engine::Time;
using Engine::Input;
using Engine::Scene;
using Engine::Entity;
using Engine::TransformComponent;
using Engine::AssetManager;
using Engine::SpriteComponent;
using Engine::VelocityComponent;



EditorApp::EditorApp()
    : m_isRunning(true), m_Window(nullptr), m_Renderer(nullptr), m_GLContext(nullptr), m_AssetManager(std::make_unique<AssetManager>()) {
    initialize();
}

EditorApp::~EditorApp() {
    shutdown();
}

void EditorApp::initialize() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        Log::error(std::string("failed to initialize SDL: ") + SDL_GetError());
        throw std::runtime_error("Failed to initialize SDL");
    }

    m_Window = SDL_CreateWindow(
        "Solidarity Engine Editor",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!m_Window) {
        Log::error(std::string("Failed to create SDL Window: ") + SDL_GetError());
        throw std::runtime_error("Failed to create Window");
    }

    m_Renderer = SDL_CreateRenderer(m_Window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!m_Renderer) {
        Log::error(std::string("Failed to create SDL Renderer: ") + SDL_GetError());
        throw std::runtime_error("Failed to create Renderer");
    }
    
    m_AssetManager->init(m_Renderer);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(m_Window, m_Renderer);
    ImGui_ImplSDLRenderer2_Init(m_Renderer);

    Input::init();

    m_AssetManager->loadTexture("testasset", "../editor/src/testassets/arrow.png");
    SDL_Texture* testTex = m_AssetManager->getTexture("testasset");

    if (!testTex) {
         Log::warn("Failed to load 'assets/arrow.png'. Creating placeholder texture.");
         SDL_Surface* tempSurface = SDL_CreateRGBSurface(0, 128, 32, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
         if (tempSurface) {
             SDL_FillRect(tempSurface, NULL, SDL_MapRGB(tempSurface->format, 255, 100, 100));
             testTex = SDL_CreateTextureFromSurface(m_Renderer, tempSurface);
             SDL_FreeSurface(tempSurface);
             if (testTex) {
                 m_AssetManager->m_Textures["testasset"] = testTex; 
             }
         }
    }

    currentScene = std::make_unique<Scene>("Test scene");

    Entity* player = currentScene->createEntity("testikmestik");
    player->addComponent<TransformComponent>(glm::vec2(100.0f, 100.0f), 0.0f, glm::vec2(2.0f, 2.0f));
    player->addComponent<SpriteComponent>("testasset", testTex, 0, 0, 128, 32);
    player->addComponent<VelocityComponent>(glm::vec2(50.0f, 0.0f));

    currentScene->setRenderer(m_Renderer);

    currentScene->init();

    Log::info(std::string("editor initialized successfully"));
}

void EditorApp::run() {
    
    Time::update();

    while (m_isRunning) {
        Time::update();
        float dt = Time::getDeltaTime();

        processEvents();

        if (currentScene) {
            currentScene->update(dt);
        }

        update();

        render();

        Input::update();
    }
}

void EditorApp::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        if (event.type == SDL_QUIT) {
            m_isRunning = false;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_Window)) {
            m_isRunning = false;
        }
    }
}

void EditorApp::update() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    ImGui::ShowDemoWindow(); 

    if (ImGui::Begin("engine stats:")) {
        float fps = (Time::getDeltaTime() > 0.0f) ? (1.0f / Time::getDeltaTime()) : 0.0f;

        ImGui::Text("deltaTime: %.4f s", Time::getDeltaTime());
        ImGui::Text("fps: %.1f", fps);
        ImGui::Text("runningTime: %.2f s", Time::getRunningTime());

        ImGui::Separator();

        ImGui::Text("Input:");

        if (Input::isKeyDown(SDL_SCANCODE_W)) {
            ImGui::Text("W key je dole");
        }

        if (Input::isKeyPressed(SDL_SCANCODE_SPACE)) {
            Log::info("Space pressed");
        }

        SDL_Point mosuePos = Input::getMousePosition();
        ImGui::Text("mosue pos: (%d, %d)", mosuePos.x, mosuePos.y);

        if (Input::isMouseButtonPressed(SDL_BUTTON_LEFT)) {
            Log::info("Left button pressed");
        }
        
        if (currentScene) {
            ImGui::Text("Active scene: %s", currentScene->getName().c_str());
            ImGui::Text("test entity status: initialised");
        }

        ImGui::End();
    }
}

void EditorApp::render() {
    ImGui::Render();

    SDL_SetRenderDrawColor(m_Renderer, 45, 45, 45, 255); 
    SDL_RenderClear(m_Renderer);

    if (currentScene) {
        currentScene->render();
    }

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_Renderer);
    SDL_RenderPresent(m_Renderer);
}

void EditorApp::shutdown() {

    if (currentScene) {
        currentScene->shutdown();
        currentScene.reset();
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (m_Renderer) SDL_DestroyRenderer(m_Renderer);
    if (m_Window) SDL_DestroyWindow(m_Window);
    SDL_Quit();

    Log::info(std::string("editor shutdown"));
}