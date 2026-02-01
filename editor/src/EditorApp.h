#pragma once

#include "imgui.h"
#include "ImGuizmo.h"
#include "core/AssetManager.h"
#include "core/Project.h"
#include "ecs/Entity.h"
#include "imgui.h"
#include <SDL2/SDL.h>
#include <memory>

// Forward declarations to keep the header lean
namespace Engine {
class Scene;
}

class EditorApp {
  enum class SceneState { EDIT, PLAY };
  enum class AppState { BROWSER, EDITOR };

public:
  EditorApp();
  ~EditorApp();

  // Deleted copy constructor and assignment operator for safety with SDL
  // pointers
  EditorApp(const EditorApp &) = delete;
  EditorApp &operator=(const EditorApp &) = delete;

  void run();

private:
  void initialize();
  void shutdown();

  void setupBaseWindow();

  void setActiveScene(const std::string& scenePath);
  
  void onBuildButtonClicked(bool forWindows);

  void processEvents();
  void update();
  void render();

  // --- Editor Specific UI Methods ---
  void renderViewport();
  void renderMenuBar();
  void renderHierarchy();
  void renderInspector();
  void renderConsole();
  void renderAssetManager();
  void renderBuildPopup();

  void loadProjectAssets();

  // --- State ---
  bool m_isRunning;

  // UI Visibility toggles
  bool m_ShowConsole = true;
  bool m_ShowHierarchy = true;
  bool m_ShowInspector = true;
  bool m_ShowAssetManager = true;

  // --- SDL Resources ---
  SDL_Window *m_Window;
  SDL_Renderer *m_Renderer;

  /** * The texture that the game engine renders into.
   * This allows us to display the game inside an ImGui window.
   */
  SDL_Texture *m_GameRenderTarget;

  // --- Engine Systems ---
  std::unique_ptr<Engine::Scene> currentScene;
  std::unique_ptr<Engine::AssetManager> m_AssetManager;

  Engine::Entity *m_SelectedEntity = nullptr;

  bool m_SceneSelected = false;

  ImVec2 viewportSize;
  ImVec2 viewportPos;

  SceneState m_SceneState = SceneState::EDIT;
  AppState m_AppState = AppState::BROWSER;

  SDL_Texture *m_SelectedTexture = nullptr;
  std::string m_RenameTarget;
  char m_RenameBuffer[256] = "";
  std::string m_SelectedAssetId = "";
  std::unordered_map<std::string, bool> m_AssetVisibility;
  std::unordered_map<std::string, SDL_Color> m_AssetTint;
  Engine::Entity* m_EntityToDelete = nullptr;
  bool g_EditColliderMode = false;
  Engine::Project* m_currentProject = nullptr;
  ImGuizmo::OPERATION op;

  std::string activeScenePath = "";

};