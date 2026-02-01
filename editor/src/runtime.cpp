#include "SDL.h"
#include "core/AssetManager.h"
#include "core/Input.h"
#include "core/Log.h"
#include "core/Project.h"
#include "core/ProjectSerializer.h"
#include "core/Time.h"
#include "scene/Scene.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace fs = std::filesystem;

/**
 * GameApp: The Standalone Player
 * This class is the entry point for the exported game. It lacks all Editor UI
 * and focuses entirely on project loading and scene execution.
 */
class GameApp {
public:
  GameApp(const std::string &cmdLinePath) : m_isRunning(true) {
    // 1. Initialize SDL Core
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO |
                 SDL_INIT_GAMECONTROLLER) != 0) {
      showFatalError("SDL Init Failed", SDL_GetError());
      throw std::runtime_error("SDL_Init Failed");
    }

    m_Project = std::make_unique<Engine::Project>();

    // 2. Resolve Path to data.config (The exported .eproj file)
    std::string configPath = resolveProjectPath(cmdLinePath);

    if (configPath.empty() || !Engine::ProjectSerializer::loadProjectFile(
                                  m_Project.get(), configPath)) {
      showFatalError("Configuration Error",
                     "Could not find or load 'data.config'.\nEnsure the file "
                     "exists in the game directory.");
      m_isRunning = false;
      return;
    }

    // 3. Set working directory to the project root
    fs::current_path(fs::path(configPath).parent_path());
    Engine::Log::info("Runtime Root: " + fs::current_path().string());

    auto config = m_Project->getConfig();

    // 4. Create Window & Renderer
    // Allow High-DPI for sharp text and 2D assets on modern displays
    m_Window = SDL_CreateWindow(config.name.empty() ? "Engine Runtime"
                                                    : config.name.c_str(),
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                config.width > 0 ? config.width : 1280,
                                config.height > 0 ? config.height : 720,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!m_Window) {
      showFatalError("Window Error", SDL_GetError());
      return;
    }

    // Using Accelerated + VSync for smooth gameplay
    m_Renderer = SDL_CreateRenderer(
        m_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!m_Renderer) {
      showFatalError("Renderer Error", SDL_GetError());
      return;
    }

    // 5. Initialize Engine Systems
    Engine::Input::init();
    m_AssetManager = std::make_unique<Engine::AssetManager>();
    m_AssetManager->init(m_Renderer);
  }

  ~GameApp() {
    if (m_CurrentScene)
      m_CurrentScene->shutdown();
    if (m_Renderer)
      SDL_DestroyRenderer(m_Renderer);
    if (m_Window)
      SDL_DestroyWindow(m_Window);
    SDL_Quit();
  }

  void DrawSceneBackground(SDL_Renderer *renderer, Engine::Scene *scene,
                           Engine::AssetManager *assetManager, float w,
                           float h) {
    if (!scene)
      return;
    auto &bg = scene->getBackground();

    if (bg.type == Engine::BackgroundType::Solid) {
      SDL_SetRenderDrawColor(renderer, (Uint8)(bg.color1.r * 255),
                             (Uint8)(bg.color1.g * 255),
                             (Uint8)(bg.color1.b * 255), 255);
      SDL_RenderClear(renderer);
    } else if (bg.type == Engine::BackgroundType::Gradient) {
      // Simple vertical gradient pass
      for (int i = 0; i < (int)h; i++) {
        float t = (float)i / (float)h;
        glm::vec4 c = glm::mix(bg.color1, bg.color2, t);
        SDL_SetRenderDrawColor(renderer, (Uint8)(c.r * 255), (Uint8)(c.g * 255),
                               (Uint8)(c.b * 255), 255);
        SDL_RenderDrawLine(renderer, 0, i, (int)w, i);
      }
    } else if (bg.type == Engine::BackgroundType::Image) {
      SDL_Texture *tex = assetManager->getTexture(bg.assetId);
      if (tex) {
        SDL_Rect dest = {0, 0, (int)w, (int)h};
        if (!bg.stretch) {
          int tw, th;
          SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
          float screenAspect = w / h;
          float texAspect = (float)tw / th;
          if (screenAspect > texAspect) {
            dest.w = (int)(h * texAspect);
            dest.x = (int)((w - dest.w) / 2);
          } else {
            dest.h = (int)(w / texAspect);
            dest.y = (int)((h - dest.h) / 2);
          }
        }
        SDL_RenderCopy(renderer, tex, NULL, &dest);
      } else {
        // Fallback clear
        SDL_SetRenderDrawColor(renderer, 30, 30, 45, 255);
        SDL_RenderClear(renderer);
      }
    }
  }

  void run() {
    if (!m_isRunning)
      return;

    loadStartScene();

    while (m_isRunning) {
      Engine::Time::update();
      Engine::Input::update();
      processEvents();

      // Render Pass
      SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 255);
      SDL_RenderClear(m_Renderer);

      if (m_CurrentScene) {
        int w, h;
        SDL_GetWindowSize(m_Window, &w, &h);

        DrawSceneBackground(m_Renderer, m_CurrentScene.get(), m_AssetManager.get(), w, h);

        m_CurrentScene->update(Engine::Time::getDeltaTime());

        auto camera = m_CurrentScene->getSceneCamera();
        if (camera) {
          m_CurrentScene->render(m_Renderer, *camera, w, h);
        } else {
          static bool warned = false;
          m_CurrentScene->render(m_Renderer, *camera, w, h);

          if (!warned) {
            Engine::Log::warn("No Camera found in active scene.");
            warned = true;
          }
        }
      }

      SDL_RenderPresent(m_Renderer);
    }
  }

private:
  std::string resolveProjectPath(const std::string &cmdLinePath) {
    if (!cmdLinePath.empty() && fs::exists(cmdLinePath))
      return cmdLinePath;

    fs::path configInCwd = fs::current_path() / "data.config";
    if (fs::exists(configInCwd))
      return configInCwd.string();

    try {
      for (const auto &entry : fs::directory_iterator(fs::current_path())) {
        if (entry.path().filename() == "data.config")
          return entry.path().string();
      }
    } catch (...) {
    }

    return "";
  }

  void processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        m_isRunning = false;

      // Allow Alt+Enter for fullscreen or ESC to exit if specified in game
      // logic
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_ESCAPE) {
          // In a production runtime, you might want to show a menu instead
          // m_isRunning = false;
        }
      }
    }
  }

  void loadStartScene() {
    auto config = m_Project->getConfig();
    if (config.startScenePath.empty()) {
      Engine::Log::error("No start scene defined in data.config");
      return;
    }

    bool success = Engine::ProjectSerializer::loadScene(
        m_CurrentScene, m_Renderer, config.startScenePath,
        m_AssetManager.get());

    if (success && m_CurrentScene) {
      m_CurrentScene->setRenderer(m_Renderer);
      m_CurrentScene->init();
      Engine::Log::info("Runtime loaded scene: " + config.startScenePath);
    } else {
      showFatalError("Scene Load Failure",
                     "Failed to load: " + config.startScenePath);
      m_isRunning = false;
    }
  }

  void showFatalError(const std::string &title, const std::string &message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(),
                             message.c_str(), m_Window);
  }

  bool m_isRunning;
  SDL_Window *m_Window = nullptr;
  SDL_Renderer *m_Renderer = nullptr;
  std::unique_ptr<Engine::Project> m_Project;
  std::unique_ptr<Engine::Scene> m_CurrentScene;
  std::unique_ptr<Engine::AssetManager> m_AssetManager;
};

// Entry point
int main(int argc, char *argv[]) {
  try {
    std::string projectArgument = (argc > 1) ? argv[1] : "";

    GameApp app(projectArgument);
    app.run();
  } catch (const std::exception &e) {
    std::cerr << "Engine Fatal Exception: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}