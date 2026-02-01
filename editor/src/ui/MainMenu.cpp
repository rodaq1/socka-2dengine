#include "../BuildSystem.h"
#include "../EditorApp.h"
#include "core/Log.h"
#include "core/ProjectSerializer.h"
#include "scene/Scene.h"
#include <filesystem>
#include <future>
#include <imgui.h>
#include <portable-file-dialogs.h>

static bool showBuildPopupRequest = false;
void EditorApp::renderBuildPopup() {
  static char buildPath[512] = "";
  static int selectedTarget = 0;
  static bool isBuilding = false;
  static std::string buildStatus = "";
  static std::future<bool> buildFuture;

  if (showBuildPopupRequest) {
    ImGui::OpenPopup("Build Project");
    showBuildPopupRequest = false;
  }

  if (ImGui::BeginPopupModal("Build Project", NULL,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    if (!isBuilding) {
      ImGui::Text("Configure Standalone Build");
      ImGui::Separator();

      const char *targets[] = {"Windows (.exe)", "Linux"};
      ImGui::Combo("Platform", &selectedTarget, targets, IM_ARRAYSIZE(targets));

      ImGui::Separator();

      if (ImGui::Button("Build", ImVec2(120, 0))) {
        if (strlen(buildPath) > 0) {
          isBuilding = true;
          buildStatus = "Compiling and Packaging... Please wait.";

          std::string pName = m_currentProject->getProjectName();
          fs::path pRoot = m_currentProject->getPath();

          fs::path executablePath = fs::current_path();
          fs::path eRoot = BuildSystem::FindEngineRoot(executablePath);

          buildFuture = std::async(std::launch::async, [pName, pRoot, eRoot,
                                                        targetIdx =
                                                            selectedTarget]() {
            try {
              BuildSystem::Target t = (targetIdx == 0)
                                          ? BuildSystem::Target::Windows
                                          : BuildSystem::Target::Linux;

              return BuildSystem::Build(pName, pRoot, eRoot, t);
            } catch (const std::exception &e) {
              Engine::Log::error("Build Exception: " + std::string(e.what()));
              return false;
            } catch (...) {
              return false;
            }
          });
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        ImGui::CloseCurrentPopup();
      }
    } else {
      ImGui::Text("%s", buildStatus.c_str());

      static float anim = 0.0f;
      anim += ImGui::GetIO().DeltaTime;
      const char *spin = "|/-\\";
      ImGui::Text("Processing %c", spin[(int)(anim * 10.0f) % 4]);

      if (buildFuture.wait_for(std::chrono::seconds(0)) ==
          std::future_status::ready) {
        isBuilding = false;
        if (buildFuture.get()) {
          Engine::Log::info("Build successful! Exported to project folder!");
          ImGui::CloseCurrentPopup();
        } else {
          buildStatus = "Build failed! Check log.";
        }
      }
    }
    ImGui::EndPopup();
  }
}

void EditorApp::renderMenuBar() {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
        Engine::ProjectSerializer::saveProjectFile(
            m_currentProject, m_currentProject->getProjectFilePath());
      }

      ImGui::Separator();

      if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
        currentScene = std::make_unique<Engine::Scene>("New Scene");
        currentScene->setRenderer(m_Renderer);
        currentScene->init();
        m_SelectedEntity = nullptr;
        Engine::Log::info("Created new scene.");
      }

      if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
        std::string startPath = ".";
        if (m_currentProject) {
          std::filesystem::path sceneDir =
              m_currentProject->getPath() / "scenes";
          startPath = std::filesystem::exists(sceneDir)
                          ? sceneDir.string()
                          : m_currentProject->getPath().string();
        }

        auto selection =
            pfd::open_file("Select Scene", startPath,
                           {"Scene Files (.json)", "*.json", "All Files", "*"})
                .result();

        if (!selection.empty()) {
          m_SelectedEntity = nullptr;
          Engine::ProjectSerializer::loadScene(currentScene, m_Renderer,
                                               selection[0], m_AssetManager.get());
          setActiveScene(selection[0]);
          activeScenePath = selection[0];
          Engine::Log::info("Scene loaded from: " + selection[0]);

          if (m_currentProject) {
            auto &config = m_currentProject->getConfig();
            config.lastActiveScene =
                std::filesystem::relative(selection[0],
                                          m_currentProject->getPath())
                    .string();
            Engine::ProjectSerializer::saveProjectFile(
                m_currentProject, m_currentProject->getProjectFilePath());
          }
        }
      }

      if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
        if (!activeScenePath.empty()) {
          Engine::ProjectSerializer::saveScene(currentScene.get(),
                                               activeScenePath);
          Engine::Log::info("Scene saved!");
        } else {
          auto destination =
              pfd::save_file("Save Scene", currentScene->getName() + ".json",
                             {"Scene Files (.json)", "*.json"})
                  .result();
          if (!destination.empty()) {
            activeScenePath = destination;
            Engine::ProjectSerializer::saveScene(currentScene.get(),
                                                 activeScenePath);
            Engine::Log::info("Scene saved!");
          }
        }
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Build Project...", "Ctrl+B", false,
                          m_currentProject != nullptr)) {
        showBuildPopupRequest = true;
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Close Project")) {
        m_AppState = AppState::BROWSER;
        m_SelectedEntity = nullptr;
        if (currentScene)
          currentScene->shutdown();
        Engine::Log::info("Returned to Project Browser.");
      }

      if (ImGui::MenuItem("Exit", "Alt+F4")) {
        m_isRunning = false;
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {
      }
      if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Delete Entity", "Del", false,
                          m_SelectedEntity != nullptr)) {
        m_EntityToDelete = m_SelectedEntity;
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Window")) {
      ImGui::MenuItem("Hierarchy", nullptr, &m_ShowHierarchy);
      ImGui::MenuItem("Inspector", nullptr, &m_ShowInspector);
      ImGui::MenuItem("Console", nullptr, &m_ShowConsole);
      ImGui::MenuItem("Asset Manager", nullptr, &m_ShowAssetManager);
      ImGui::EndMenu();
    }

    if (currentScene) {
      float width = ImGui::GetWindowWidth();
      std::string sceneText = "Active Scene: " + currentScene->getName();
      float textWidth = ImGui::CalcTextSize(sceneText.c_str()).x;
      ImGui::SameLine(width - textWidth - 20);
      ImGui::TextDisabled("%s", sceneText.c_str());
    }

    ImGui::EndMainMenuBar();
  }
  renderBuildPopup();
}