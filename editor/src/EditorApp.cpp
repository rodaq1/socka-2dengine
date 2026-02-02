#include "EditorApp.h"
#include "SDL_events.h"
#include "SDL_render.h"
#include "core/Input.h"
#include "core/Log.h"
#include "core/ProjectSerializer.h"
#include "core/Time.h"
#include "ecs/Entity.h"
#include "ecs/components/SpriteComponent.h"
#include "ecs/components/TransformComponent.h"
#include "scene/Scene.h"
#include "ui/Console.h"
#include "ui/Global.h"
#include "ui/ProjectBrowser.h"

#include "ImGuizmo.h"
#include "ui/Gizmos.h"
#include <algorithm>
#include <filesystem>
#include <future>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <stdexcept>

static ProjectBrowser g_ProjectBrowser;

ImFont *g_FontTitle = nullptr;
ImFont *g_FontHeading = nullptr;
ImFont *g_FontDefault = nullptr;

EditorApp::EditorApp()
    : m_isRunning(true), m_ShowConsole(true), m_ShowHierarchy(true),
      m_ShowInspector(true), m_Window(nullptr), m_Renderer(nullptr),
      m_GameRenderTarget(nullptr),
      m_AssetManager(std::make_unique<Engine::AssetManager>()),
      m_AppState(AppState::BROWSER) {

  setupBaseWindow();
}

EditorApp::~EditorApp() { shutdown(); }

void InitBrowserFonts() {
  ImGuiIO &io = ImGui::GetIO();

  g_FontDefault = io.Fonts->AddFontDefault();

  float baseSize = 18.0f;

  g_FontTitle = io.Fonts->AddFontFromFileTTF(
      "../editor/src/ui/fonts/Roboto-Bold.ttf", 32.0f);
  if (!g_FontTitle)
    g_FontTitle = g_FontDefault;

  g_FontHeading = io.Fonts->AddFontFromFileTTF(
      "../editor/src/ui/fonts/Roboto-Medium.ttf", 22.0f);
  if (!g_FontHeading)
    g_FontHeading = g_FontDefault;

  g_FontDefault = io.Fonts->AddFontFromFileTTF(
      "../editor/src/ui/fonts/Roboto-Regular.ttf", 14.0f);

  io.Fonts->Build();
}

void EditorApp::setupBaseWindow() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
    throw std::runtime_error("Failed to initialize SDL");
  }

  m_Window = SDL_CreateWindow("Solidarity Engine", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 1920, 1080,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  m_Renderer =
      SDL_CreateRenderer(m_Window, -1,
                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE |
                             SDL_RENDERER_PRESENTVSYNC);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForSDLRenderer(m_Window, m_Renderer);
  ImGui_ImplSDLRenderer2_Init(m_Renderer);
  InitBrowserFonts();

  Engine::Input::init();
  g_ProjectBrowser.refreshProjectList("./projects");
  Engine::Log::SetCallback([](std::string_view msg, Engine::Log::Level level) {
        ImVec4 color;
        switch (level) {
            case Engine::Log::Level::Error:
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Soft Red
                break;
            case Engine::Log::Level::Warn:
                color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); // Orange/Yellow
                break;
            case Engine::Log::Level::Info:
            default:
                color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // Off-White
                break;
        }
        
        // Route the engine message to our UI Console
        Console::addLog(msg, color);
    });
}

void EditorApp::setActiveScene(const std::string &scenePath) {
  Engine::Log::info(scenePath);
  activeScenePath = scenePath;

  if (m_currentProject) {
    auto &config = m_currentProject->getConfig();

    std::filesystem::path relativePath =
        std::filesystem::relative(scenePath, m_currentProject->getPath());
    config.lastActiveScene = relativePath.string();
    Engine::ProjectSerializer::saveProjectFile(
        m_currentProject, m_currentProject->getProjectFilePath());
  }
}

void EditorApp::initialize() {
  if (!m_currentProject) {
    Engine::Log::error("EditorApp::initialize failed: No project loaded.");
    return;
  }

  m_GameRenderTarget = SDL_CreateTexture(m_Renderer, SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET, 1280, 720);

  m_AssetManager->init(m_Renderer);
  m_AssetManager->clearInstanceAssets();

  std::filesystem::path projectRoot = m_currentProject->getPath();
  std::filesystem::path assetsRoot = projectRoot / "assets";

  Engine::ProjectConfig config = m_currentProject->getConfig();

  std::string sceneToLoad = config.lastActiveScene.empty()
                                ? config.startScenePath
                                : config.lastActiveScene;
  std::filesystem::path scenePath(sceneToLoad);

  std::filesystem::path fullScenePath;

  if (scenePath.is_absolute()) {
    fullScenePath = scenePath;
  } else if (sceneToLoad.find("scenes") == std::string::npos) {
    fullScenePath = m_currentProject->getPath() / "scenes" / scenePath;
  } else {
    fullScenePath = m_currentProject->getPath() / scenePath;
  }

  Engine::Log::info("Project: " + config.name);
  Engine::Log::info("Attempting to load scene: " + fullScenePath.string());

  if (!sceneToLoad.empty() && std::filesystem::exists(fullScenePath)) {
    Engine::ProjectSerializer::loadScene(currentScene, m_Renderer,
                                         fullScenePath.string(), m_AssetManager.get());

    if (currentScene) {
      currentScene->setRenderer(m_Renderer);

      config.lastActiveScene = fullScenePath.filename().string();
      m_currentProject->setConfig(config);

      setActiveScene(fullScenePath.string());
    }
  } else {
    Engine::Log::warn(
        "Target scene not found. Creating a blank default scene.");

    currentScene = std::make_unique<Engine::Scene>("main");
    currentScene->setRenderer(m_Renderer);
    currentScene->init();

    std::filesystem::path defaultPath =
        m_currentProject->getPath() / "scenes" / "main.json";

    config.lastActiveScene = defaultPath.filename().string();
    m_currentProject->setConfig(config);

    setActiveScene(defaultPath.string());
  }

  Engine::Log::info("Project systems and scene initialized successfully.");
}

void EditorApp::run() {
  while (m_isRunning) {
    Engine::Time::update();
    Engine::Input::update();

    processEvents();

    if (m_AppState == AppState::BROWSER) {
      SDL_SetRenderDrawColor(m_Renderer, 30, 30, 30, 255);
      SDL_RenderClear(m_Renderer);

      ImGui_ImplSDLRenderer2_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();
      m_currentProject = g_ProjectBrowser.render();
      if (m_currentProject) {
        initialize();
        loadProjectAssets();
        m_AppState = AppState::EDITOR;
      }

      ImGui::Render();
      ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_Renderer);

      if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
      }
      SDL_RenderPresent(m_Renderer);
    } else if (m_AppState == AppState::EDITOR) {
      if (m_EntityToDelete) {
        if (m_SelectedEntity == m_EntityToDelete)
          m_SelectedEntity = nullptr;
      }

      update();

      render();

      if (m_EntityToDelete && currentScene) {
        currentScene->destroyEntity(m_EntityToDelete);
        m_EntityToDelete = nullptr;
      }
    }
  }
}

void EditorApp::processEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    if (event.type == SDL_QUIT)
      m_isRunning = false;

    if (m_AppState == AppState::EDITOR) {

      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_T:
          currentOperation = GizmoOperation::TRANSLATE;
          break;
        case SDL_SCANCODE_R:
          currentOperation = GizmoOperation::ROTATE;
          break;
        case SDL_SCANCODE_S:
          currentOperation = GizmoOperation::SCALE;
          break;
        default:
          break;
        }
      }
      // Mouse Picking
      if (event.type == SDL_MOUSEBUTTONDOWN &&
          event.button.button == SDL_BUTTON_LEFT) {
        if (ImGuizmo::IsOver() || ImGui::GetIO().WantCaptureMouse)
          continue;

        ImVec2 mousePos = ImGui::GetIO().MousePos;
        if (mousePos.x >= viewportPos.x &&
            mousePos.x <= viewportPos.x + viewportSize.x &&
            mousePos.y >= viewportPos.y &&
            mousePos.y <= viewportPos.y + viewportSize.y) {

          // Conversion to local viewport space
          float worldMouseX = mousePos.x - viewportPos.x;
          float worldMouseY = mousePos.y - viewportPos.y;

          m_SelectedEntity = nullptr;
          auto entities = currentScene->getEntityRawPointers();
          for (int i = (int)entities.size() - 1; i >= 0; i--) {
            auto *entity = entities[i];
            if (!entity->hasComponent<Engine::TransformComponent>())
              continue;

            auto tr = entity->getComponent<Engine::TransformComponent>();
            glm::vec2 size = {32, 32}; // Default if no sprite
            if (entity->hasComponent<Engine::SpriteComponent>()) {
              auto sp = entity->getComponent<Engine::SpriteComponent>();
              size = {(float)sp->sourceRect.w * tr->scale.x,
                      (float)sp->sourceRect.h * tr->scale.y};
            }

            if (worldMouseX >= tr->position.x &&
                worldMouseX <= tr->position.x + size.x &&
                worldMouseY >= tr->position.y &&
                worldMouseY <= tr->position.y + size.y) {
              m_SelectedEntity = entity;
              break;
            }
          }
        }
      }
    }
  }
}

void EditorApp::update() {
  if (currentScene && m_SceneState == SceneState::PLAY) {
    currentScene->update(Engine::Time::getDeltaTime());
  }
}

void EditorApp::render() {
  ImGui_ImplSDLRenderer2_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
  ImGuizmo::BeginFrame();

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::DockSpaceOverViewport(viewport->ID);

  renderMenuBar();

  if (m_ShowHierarchy)
    renderHierarchy();
  if (m_ShowInspector)
    renderInspector();
  if (m_ShowConsole)
    Console::render();
  if (m_ShowAssetManager)
    renderAssetManager();

  renderViewport();

  SDL_SetRenderTarget(m_Renderer, nullptr);
  SDL_SetRenderDrawColor(m_Renderer, 45, 45, 45, 255);
  SDL_RenderClear(m_Renderer);

  ImGui::Render();
  ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), m_Renderer);

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  SDL_RenderPresent(m_Renderer);
}

void EditorApp::shutdown() {
  if (currentScene)
    currentScene->shutdown();
  ImGui_ImplSDLRenderer2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  if (m_GameRenderTarget)
    SDL_DestroyTexture(m_GameRenderTarget);
  if (m_Renderer)
    SDL_DestroyRenderer(m_Renderer);
  if (m_Window)
    SDL_DestroyWindow(m_Window);
  SDL_Quit();
}
#include "BuildSystem.h"
#include <filesystem>

void EditorApp::onBuildButtonClicked(bool forWindows) {
  BuildSystem::Target target =
      forWindows ? BuildSystem::Target::Windows : BuildSystem::Target::Linux;

  std::string projectName = m_currentProject->getConfig().name;
  fs::path projectRoot = m_currentProject->getPath();
  fs::path engineRoot = "/path/to/engine";

  if (BuildSystem::Build(projectName, projectRoot, engineRoot, target)) {
    Engine::Log::info("Build success!");
  } else {
    Engine::Log::error("Build failed. Check console output.");
  }
}