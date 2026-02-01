#include "ProjectBrowser.h"
#include "Global.h"
#include "core/FileSystem.h"
#include "core/Log.h"
#include "core/Project.h"
#include "imgui.h"
#include "portable-file-dialogs.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace ImGui {
void HelpMarker(const char *desc) {
  ImGui::TextDisabled("(?)");
  if (ImGui::IsItemHovered()) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
    ImGui::TextUnformatted(desc);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }
}
} // namespace ImGui

static std::vector<fs::path> getUserSearchPaths() {
  std::vector<fs::path> paths;

#ifdef _WIN32
  char *userProfile = std::getenv("USERPROFILE");
  if (userProfile) {
    paths.emplace_back(fs::path(userProfile) / "Documents");
    paths.emplace_back(fs::path(userProfile) / "Desktop");
  }
#else
  char *home = std::getenv("HOME");
  if (home) {
    paths.emplace_back(fs::path(home) / "Documents");
  }
#endif

  return paths;
}

void ProjectBrowser::refreshProjectList(const std::string &) {
  m_FoundProjects.clear();
  m_SelectedIndex = -1;

  auto roots = getUserSearchPaths();
  auto options = fs::directory_options::skip_permission_denied;

  try {
    for (const auto &root : roots) {
      if (!fs::exists(root))
        continue;

      for (const auto &entry :
           fs::recursive_directory_iterator(root, options)) {
        if (!entry.is_regular_file())
          continue;

        if (entry.path().extension() != Engine::Project::Extension)
          continue;

        Engine::Project project;
        // Store the directory containing the .eproj file
        project.setPath(entry.path().string());

        std::string stemName = entry.path().stem().string();
        project.setName(stemName);

        m_FoundProjects.push_back(project);
      }
    }

    std::sort(m_FoundProjects.begin(), m_FoundProjects.end(),
              [](const Engine::Project &a, const Engine::Project &b) {
                return a.getProjectName() < b.getProjectName();
              });

    Engine::Log::info("Discovery complete. Found " +
                      std::to_string(m_FoundProjects.size()) + " projects.");
  } catch (const std::exception &e) {
    Engine::Log::error("Failed to scan for projects: " + std::string(e.what()));
  }
}

void CenterText(const char *text) {
  auto windowWidth = ImGui::GetWindowSize().x;
  auto textWidth = ImGui::CalcTextSize(text).x;
  ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
  ImGui::Text("%s", text);
}

Engine::Project *ProjectBrowser::render() {
  Engine::Project *projectOpened = nullptr;
  static int projectToModifyIndex =
      -1; // Track which project the context menu is targeting

  static bool styleInitialized = false;
  if (!styleInitialized) {
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(15, 15);
    style.WindowRounding = 12.0f;
    style.FrameRounding = 6.0f;
    style.ItemSpacing = ImVec2(10, 10);

    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.18f, 0.35f, 0.66f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.45f, 0.85f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.18f, 0.21f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);

    styleInitialized = true;
  }

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing,
                          ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(800, 520), ImGuiCond_FirstUseEver);

  if (g_FontDefault)
    ImGui::PushFont(g_FontDefault);

  ImGui::Begin("Project Browser", nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoCollapse);

  // --- LOGO / TITLE ---
  if (g_FontTitle)
    ImGui::PushFont(g_FontTitle);
  ImGui::Spacing();
  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 0.65f, 1.0f, 1.0f));
  CenterText("SOLIDARITY");
  ImGui::PopStyleColor();
  if (g_FontTitle)
    ImGui::PopFont();

  ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.55f, 1.0f));
  CenterText("E N G I N E   W O R K S P A C E");
  ImGui::PopStyleColor();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  float leftColWidth = 280.0f;

  // --- LEFT SIDEBAR ---
  ImGui::BeginChild("SideBar", ImVec2(leftColWidth, 0), true,
                    ImGuiWindowFlags_NoScrollbar);
  ImGui::TextDisabled("PROJECTS");
  ImGui::Spacing();

  float footerHeight = 40.0f;
  ImVec2 sidebarAvail = ImGui::GetContentRegionAvail();

  ImGui::BeginChild("ProjectsList", ImVec2(0, sidebarAvail.y - footerHeight),
                    false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

  if (m_FoundProjects.empty()) {
    ImGui::TextWrapped("No projects found.");
  } else {
    for (int i = 0; i < (int)m_FoundProjects.size(); ++i) {
      ImGui::PushID(i);
      bool selected = m_SelectedIndex == i;
      const std::string &name = m_FoundProjects[i].getProjectName().empty()
                                    ? "Unnamed"
                                    : m_FoundProjects[i].getProjectName();

      if (ImGui::Selectable(name.c_str(), selected, 0, ImVec2(0, 32))) {
        m_SelectedIndex = i;
      }

      if (ImGui::BeginPopupContextItem("ProjectContextMenu")) {
        projectToModifyIndex = i;
        if (ImGui::MenuItem("Open in Explorer")) {
          pfd::open_file("Open Folder", m_FoundProjects[i].getPath());
        }
        ImGui::Separator();
        ImGui::EndPopup();
      }

      if (ImGui::BeginPopupModal("Delete Confirmation", nullptr,
                                 ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to permanently delete:");
        ImGui::TextColored(
            ImVec4(1, 0.4f, 0.4f, 1), "%s",
            m_FoundProjects[projectToModifyIndex].getProjectName().c_str());
        ImGui::TextDisabled(
            "Path: %s",
            m_FoundProjects[projectToModifyIndex].getPath().c_str());
        ImGui::Spacing();
        ImGui::Text("This will delete the entire folder and all assets.");
        ImGui::Separator();

        if (ImGui::Button("DELETE", ImVec2(120, 0))) {
          removeProject(m_FoundProjects[projectToModifyIndex]);
          m_SelectedIndex = -1;
          projectToModifyIndex = -1;
          ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("CANCEL", ImVec2(120, 0))) {
          projectToModifyIndex = -1;
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      ImGui::PopID();
    }
  }
  ImGui::EndChild();

  ImGui::Spacing();
  if (ImGui::Button("REFRESH LIST", ImVec2(-FLT_MIN, 32))) {
    refreshProjectList(m_SearchPath);
  }
  ImGui::EndChild();

  ImGui::SameLine();

  // --- MAIN CONTENT AREA ---
  ImGui::BeginChild("MainArea", ImVec2(0, 0), false);

  const float horizontalPadding = 30.0f;
  const float bottomPadding = 40.0f;
  const float contentWidth =
      ImGui::GetContentRegionAvail().x - (horizontalPadding * 2.0f);

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + horizontalPadding);

  ImGui::BeginGroup();
  ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + contentWidth);
  ImGui::PushItemWidth(contentWidth);

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 12));

  if (m_SelectedIndex != -1) {
    auto *proj = &m_FoundProjects[m_SelectedIndex];

    ImGui::Spacing();
    if (g_FontHeading)
      ImGui::PushFont(g_FontHeading);
    ImGui::TextUnformatted(proj->getProjectName().c_str());
    if (g_FontHeading)
      ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.55f, 1.0f));
    ImGui::TextWrapped("%s", proj->getPath().c_str());
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTable("Details", 2, ImGuiTableFlags_SizingFixedFit)) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("Version");
      ImGui::TableNextColumn();
      ImGui::TextDisabled("%s", proj->getConfig().engineVersion.c_str());

      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::Text("Scene");
      ImGui::TableNextColumn();
      ImGui::TextDisabled("%s", proj->getConfig().startScenePath.c_str());

      ImGui::EndTable();
    }

    float actionButtonsHeight = 40.0f + 32.0f + ImGui::GetStyle().ItemSpacing.y;
    float dummyHeight = ImGui::GetContentRegionAvail().y -
                        (actionButtonsHeight + bottomPadding);
    if (dummyHeight > 0)
      ImGui::Dummy(ImVec2(0, dummyHeight));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.40f, 0.75f, 1.00f));
    if (ImGui::Button("OPEN PROJECT", ImVec2(contentWidth, 40.0f))) {
      Engine::FileSystem::setProjectRoot(proj->getPath());
      projectOpened = proj;
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.30f, 0.30f, 1.00f));
if (ImGui::Button("DELETE PROJECT", ImVec2(contentWidth, 40.0f))) {
    ImGui::OpenPopup("Confirm Delete?");
}
ImGui::PopStyleColor();

if (ImGui::BeginPopupModal("Confirm Delete?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    
    ImGui::PushFont(g_FontHeading);
    ImGui::TextColored(ImVec4(0.85f, 0.30f, 0.30f, 1.00f), "Warning: Permanent Action");
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0, 5));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 5));

    ImGui::PushFont(g_FontDefault);
    ImGui::Text("This action cannot be undone!");
    ImGui::Text("Are you sure you want to delete this project?");
    
    ImGui::Dummy(ImVec2(0, 15));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.30f, 0.30f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.00f, 0.40f, 0.40f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.20f, 0.20f, 1.00f));
    
    if (ImGui::Button("Yes, Delete", ImVec2(120, 35))) {
        removeProject(*proj);
        ImGui::CloseCurrentPopup();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 35))) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::PopFont(); 
    ImGui::EndPopup();
}

    if (ImGui::Button("CANCEL", ImVec2(contentWidth, 32))) {
      m_SelectedIndex = -1;
    }

  } else {
    static bool pathInitialized = false;
    if (!pathInitialized) {
      strcpy(m_NewProjectPath, m_SearchPath.c_str());
      pathInitialized = true;
    }

    ImGui::Spacing();
    if (g_FontHeading)
      ImGui::PushFont(g_FontHeading);
    ImGui::Text("Create New Project");
    if (g_FontHeading)
      ImGui::PopFont();
    ImGui::Spacing();

    ImGui::TextDisabled("PROJECT NAME");
    ImGui::InputText("##Name", m_NewProjectName, 256);

    ImGui::Spacing();
    ImGui::TextDisabled("DIRECTORY PATH");

    float buttonWidth = 90.0f;
    float spacing = ImGui::GetStyle().ItemSpacing.x;

    ImGui::SetNextItemWidth(contentWidth - buttonWidth - spacing);
    ImGui::InputText("##Path", m_NewProjectPath, 512);

    ImGui::SameLine();
    if (ImGui::Button("Browse...",
                      ImVec2(buttonWidth, ImGui::GetFrameHeight()))) {
      auto selection = pfd::select_folder("Select Project Directory").result();
      if (!selection.empty()) {
        std::strncpy(m_NewProjectPath, selection.c_str(), 255);
        m_NewProjectPath[255] = '\0';
      }
    }

    float buttonHeight = 40.0f;
    float dummyHeight =
        ImGui::GetContentRegionAvail().y - (buttonHeight + bottomPadding);
    if (dummyHeight > 0)
      ImGui::Dummy(ImVec2(0, dummyHeight));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.40f, 0.20f, 1.0f));
    if (ImGui::Button("CREATE AND LAUNCH",
                      ImVec2(contentWidth, buttonHeight))) {
      if (strlen(m_NewProjectName) > 0)
        projectOpened = createNewProject();
    }
    ImGui::PopStyleColor();
  }

  ImGui::PopStyleVar(2);
  ImGui::PopItemWidth();
  ImGui::PopTextWrapPos();
  ImGui::EndGroup();

  ImGui::EndChild();
  ImGui::End();

  if (g_FontDefault)
    ImGui::PopFont();

  return projectOpened;
}

void ProjectBrowser::removeProject(const Engine::Project &project) {
  try {
    fs::path projectDir(project.getPath());

    if (fs::exists(projectDir) && fs::is_directory(projectDir)) {
      bool containsProjectFile = false;
      for (const auto &entry : fs::directory_iterator(projectDir)) {
        if (entry.path().extension() == Engine::Project::Extension) {
          containsProjectFile = true;
          break;
        }
      }

      if (containsProjectFile) {
        fs::remove_all(projectDir);
        Engine::Log::info("Deleted project directory: " + projectDir.string());
      } else {
        Engine::Log::warn("Deletion aborted: Targeted directory is not a valid "
                          "project folder. " +
                          projectDir.string());
      }

      refreshProjectList(m_SearchPath);
    }
  } catch (const std::exception &e) {
    Engine::Log::error("Failed to delete project folder: " +
                       std::string(e.what()));
  }
}

Engine::Project *ProjectBrowser::createNewProject() {
  try {
    fs::path rootPath = fs::path(m_NewProjectPath) / m_NewProjectName;

    Engine::Project newProject;
    newProject.setName(m_NewProjectName);
    auto config = newProject.getConfig();

    fs::create_directories(rootPath / config.assetDirectory / "textures");
    fs::create_directories(rootPath / config.assetDirectory / "scripts");
    fs::create_directories(rootPath / "scenes");

    fs::path projectFilePath =
        rootPath / (std::string(m_NewProjectName) + Engine::Project::Extension);

    newProject.setPath(rootPath.string());

    std::ofstream projectFile(projectFilePath);
    if (!projectFile.is_open()) {
      Engine::Log::error("Could not create project file at: " +
                         projectFilePath.string());
      return nullptr;
    }

    projectFile << "{\n";
    projectFile << "  \"projectName\": \"" << m_NewProjectName << "\",\n";
    projectFile << "  \"engineVersion\": \"" << config.engineVersion << "\",\n";
    projectFile << "  \"assetDirectory\": \"" << config.assetDirectory
                << "\",\n";
    projectFile << "  \"startScene\": \"" << config.startScenePath << "\"\n";
    projectFile << "}";
    projectFile.close();

    Engine::Log::info("Project created successfully at: " + rootPath.string());
    Engine::FileSystem::setProjectRoot(rootPath.string());

    refreshProjectList(m_SearchPath);

    for (auto &project : m_FoundProjects) {
      if (project.getPath() == rootPath.string()) {
        return &project;
      }
    }
    return nullptr;

  } catch (const std::exception &e) {
    Engine::Log::error("Failed to create project: " + std::string(e.what()));
    return nullptr;
  }
}