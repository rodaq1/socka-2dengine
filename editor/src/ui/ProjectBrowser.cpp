#include "ProjectBrowser.h"
#include "Global.h"
#include "core/FileSystem.h"
#include "core/Log.h"
#include "core/Project.h"
#include "imgui.h"
#include "portable-file-dialogs.h"
#include <algorithm>
#include <cstring>
#include "core/ProjectSerializer.h"
#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

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

static std::vector<fs::path> getUserSearchPaths(const std::string& extraPath) {
    std::vector<fs::path> paths;

    // 1. Add the custom UI path
    if (!extraPath.empty()) {
        std::error_code ec;
        fs::path absPath = fs::absolute(extraPath, ec);
        if (!ec && fs::exists(absPath)) paths.emplace_back(absPath);
    }

    // 2. Add standard OS locations
#ifdef _WIN32
    PWSTR pathTmp = NULL;
    // Get actual Documents path (handles OneDrive/Redirects)
    if (SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &pathTmp) == S_OK) {
        paths.emplace_back(pathTmp);
        CoTaskMemFree(pathTmp);
    }
    // Get Desktop path
    if (SHGetKnownFolderPath(FOLDERID_Desktop, 0, NULL, &pathTmp) == S_OK) {
        paths.emplace_back(pathTmp);
        CoTaskMemFree(pathTmp);
    }
#else
    char* home = std::getenv("HOME");
    if (home) {
        paths.emplace_back(fs::path(home) / "Documents");
        paths.emplace_back(fs::path(home) / "Desktop");
    }
#endif

    return paths;
}

void ProjectBrowser::refreshProjectList(const std::string& currentSearchPath) {
    m_FoundProjects.clear();
    m_SelectedIndex = -1;

    auto roots = getUserSearchPaths(currentSearchPath);
    
    std::string targetExt = Engine::Project::Extension;
    if (targetExt.empty() || targetExt[0] != '.') targetExt = "." + targetExt;
    std::transform(targetExt.begin(), targetExt.end(), targetExt.begin(), ::tolower);

    for (const auto& root : roots) {
        if (!fs::exists(root)) continue;
        Engine::Log::info("Scanning root: " + root.string());

        std::error_code root_ec;
        for (const auto& entry : fs::directory_iterator(root, fs::directory_options::skip_permission_denied, root_ec)) {
            
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == targetExt) {
                    Engine::Project p;
                    p.setPath(entry.path().parent_path().string());
                    p.setName(entry.path().stem().string());
                    m_FoundProjects.push_back(p);
                    continue; 
                }
            }

            if (entry.is_directory()) {
                std::error_code sub_ec;
                for (const auto& subEntry : fs::directory_iterator(entry.path(), fs::directory_options::skip_permission_denied, sub_ec)) {
                    if (subEntry.is_regular_file()) {
                        std::string ext = subEntry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                        if (ext == targetExt) {
                            Engine::Project p;
                            p.setPath(subEntry.path().parent_path().string());
                            p.setName(subEntry.path().stem().string());
                            m_FoundProjects.push_back(p);
                        }
                    }
                }
            }
        }
        if (root_ec) Engine::Log::warn("Scan warning in " + root.string() + ": " + root_ec.message());
    }

    std::sort(m_FoundProjects.begin(), m_FoundProjects.end(), [](const Engine::Project& a, const Engine::Project& b) {
        return a.getProjectName() < b.getProjectName();
    });
    
    auto last = std::unique(m_FoundProjects.begin(), m_FoundProjects.end(), [](const Engine::Project& a, const Engine::Project& b) {
        return a.getPath() == b.getPath();
    });
    m_FoundProjects.erase(last, m_FoundProjects.end());

    Engine::Log::info("Scan finished. Total projects: " + std::to_string(m_FoundProjects.size()));
}


void CenterText(const char *text) {
  auto windowWidth = ImGui::GetWindowSize().x;
  auto textWidth = ImGui::CalcTextSize(text).x;
  ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
  ImGui::Text("%s", text);
}

Engine::Project *ProjectBrowser::render() {
  Engine::Project *projectOpened = nullptr;
  static int projectToModifyIndex = -1;

  static bool styleInitialized = false;
  
  if (!styleInitialized) {
    if (ImGui::GetCurrentContext() != nullptr) {
      ImGuiStyle &style = ImGui::GetStyle();
      style.WindowPadding = ImVec2(20, 20);
      style.FramePadding = ImVec2(12, 8);
      style.CellPadding = ImVec2(8, 6);
      style.ItemSpacing = ImVec2(8, 12);
      style.ItemInnerSpacing = ImVec2(8, 6);
      style.WindowRounding = 16.0f;
      style.FrameRounding = 8.0f;
      style.PopupRounding = 12.0f;
      style.ScrollbarRounding = 4.0f;
      style.GrabRounding = 8.0f;
      style.TabRounding = 8.0f;
      
      ImVec4 *colors = style.Colors;
      colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);
      colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
      colors[ImGuiCol_Border] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
      colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
      colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
      colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.23f, 1.00f);
      colors[ImGuiCol_TitleBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
      colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
      colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.65f, 1.00f, 1.00f);
      colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.65f, 1.00f, 1.00f);
      colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.75f, 1.00f, 1.00f);
      colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
      colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.33f, 1.00f);
      colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
      colors[ImGuiCol_Header] = ImVec4(0.22f, 0.40f, 0.75f, 0.31f);
      colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.45f, 0.85f, 0.80f);
      colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.50f, 0.95f, 1.00f);
      colors[ImGuiCol_Separator] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
      colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
      colors[ImGuiCol_SeparatorActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
      colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.20f, 0.25f, 0.00f);
      colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.30f, 0.35f, 0.67f);
      colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.40f, 0.45f, 0.95f);
      colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
      colors[ImGuiCol_TabHovered] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
      colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
      colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
      colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.13f, 0.15f, 1.00f);
      colors[ImGuiCol_TextSelectedBg] = ImVec4(0.22f, 0.40f, 0.75f, 0.35f);
      colors[ImGuiCol_NavHighlight] = ImVec4(0.40f, 0.65f, 1.00f, 1.00f);
      
      styleInitialized = true;
    }
  }

  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(1200, 750), ImGuiCond_Appearing);

  ImGui::PushID("ProjectBrowserMainWindow");

  if (g_FontDefault)
    ImGui::PushFont(g_FontDefault);

  bool windowOpen = true;
  ImGui::Begin("Project Browser###ProjectBrowserMain", &windowOpen,
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
  
  {
    ImGui::Spacing();
    ImGui::Spacing();
    
    if (g_FontTitle)
      ImGui::PushFont(g_FontTitle);
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.65f, 1.00f, 1.00f));
    CenterText("SOLIDARITY");
    ImGui::PopStyleColor();
    
    if (g_FontTitle)
      ImGui::PopFont();
    
    ImGui::Spacing();
    
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.60f, 0.70f, 0.80f));
    if (g_FontDefault)
      ImGui::PushFont(g_FontDefault);
    CenterText("ENGINE WORKSPACE");
    if (g_FontDefault)
      ImGui::PopFont();
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
  }
  
  float availableHeight = ImGui::GetContentRegionAvail().y - 10;
  
  ImGui::PushID("MainContentArea");
  ImGui::BeginChild("MainContentChild", ImVec2(0, availableHeight), false);
  
  ImGui::Columns(2, "MainColumns", false);
  float firstColumnWidth = 350.0f;
  ImGui::SetColumnWidth(0, firstColumnWidth);
  
  {
    ImGui::PushID("LeftColumn");
    ImGui::BeginChild("LeftColumnChild", ImVec2(0, 0), true);
    
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.65f, 1.00f, 1.00f));
    ImGui::Text("PROJECTS");
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    float listHeight = ImGui::GetContentRegionAvail().y - 60;
    
    ImGui::BeginChild("ProjectsListChild", ImVec2(0, listHeight), false, 
                     ImGuiWindowFlags_NoScrollbar);
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 8));
    
    if (m_FoundProjects.empty()) {
      ImGui::Dummy(ImVec2(0, 20));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.60f, 0.80f));
      ImGui::TextWrapped("No projects found in the selected directory.");
      ImGui::Spacing();
      ImGui::TextWrapped("Click 'Refresh Projects' to scan again.");
      ImGui::PopStyleColor();
    } else {
      // Use a unique string ID for the list to prevent conflicts
      static const char* listID = "ProjectsListItems";
      ImGui::PushID(listID);
      
      for (int i = 0; i < (int)m_FoundProjects.size(); ++i) {
        // Use a combination of index and project name for unique ID
        std::string itemID = "ProjectItem_" + std::to_string(i) + "_" + m_FoundProjects[i].getProjectName();
        ImGui::PushID(itemID.c_str());
        
        bool selected = m_SelectedIndex == i;
        const std::string &name = m_FoundProjects[i].getProjectName().empty()
                                    ? "Unnamed Project"
                                    : m_FoundProjects[i].getProjectName();
        
        // Calculate card dimensions
        float cardWidth = ImGui::GetContentRegionAvail().x - 16;
        if (cardWidth < 10) cardWidth = 10;
        
        // Card background
        ImVec2 cursorStart = ImGui::GetCursorPos();
        ImVec2 cursorScreenStart = ImGui::GetCursorScreenPos();
        
        // Use a unique selectable ID
        std::string selectableID = "##ProjectSelectable_" + std::to_string(i);
        if (ImGui::Selectable(selectableID.c_str(), selected, 
                             ImGuiSelectableFlags_AllowDoubleClick, 
                             ImVec2(cardWidth, 72))) {
          m_SelectedIndex = i;
          if (ImGui::IsMouseDoubleClicked(0)) {
            Engine::FileSystem::setProjectRoot(m_FoundProjects[i].getPath());
            projectOpened = &m_FoundProjects[i];
          }
        }
        
        // Draw custom card background
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 min = cursorScreenStart;
        ImVec2 max = ImVec2(min.x + cardWidth, min.y + 72);
        
        if (selected) {
          draw_list->AddRectFilled(min, max, ImColor(0.22f, 0.40f, 0.75f, 0.15f), 8.0f);
          draw_list->AddRect(min, max, ImColor(0.40f, 0.65f, 1.00f, 0.30f), 8.0f, 0, 2.0f);
        } else if (ImGui::IsItemHovered()) {
          draw_list->AddRectFilled(min, max, ImColor(0.15f, 0.15f, 0.20f, 0.50f), 8.0f);
        }
        
        // Project content
        ImGui::SetCursorPos(cursorStart);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 16);
        
        // Icon
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.65f, 1.00f, 1.00f));
        ImGui::Text("◉");
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2);
        
        // Text content
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.96f, 0.98f, 1.00f));
        ImGui::Text("%s", name.c_str());
        ImGui::PopStyleColor();
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.60f, 0.70f, 0.70f));
        ImGui::SetWindowFontScale(0.85f);
        ImGui::TextWrapped("%s", m_FoundProjects[i].getPath().filename().string().c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();
        ImGui::EndGroup();
        
        // Context menu with unique ID
        std::string contextMenuID = "ProjectContextMenu_" + std::to_string(i);
        if (ImGui::BeginPopupContextItem(contextMenuID.c_str())) {
          projectToModifyIndex = i;
          if (ImGui::MenuItem("Open Project", "Enter")) {
            Engine::FileSystem::setProjectRoot(m_FoundProjects[i].getPath());
            projectOpened = &m_FoundProjects[i];
          }
          if (ImGui::MenuItem("Show in Explorer", "Ctrl+E")) {
            pfd::open_file("Open Folder", m_FoundProjects[i].getPath().string());
          }
          ImGui::Separator();
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.40f, 0.40f, 1.00f));
          if (ImGui::MenuItem("Delete Project...", "Del")) {
            ImGui::OpenPopup("Confirm Delete?");
          }
          ImGui::PopStyleColor();
          ImGui::EndPopup();
        }
        
        ImGui::PopID(); // itemID
      }
      
      ImGui::PopID(); // listID
    }
    
    ImGui::PopStyleVar();
    ImGui::EndChild(); // ProjectsListChild
    
    // Refresh button at bottom of left column
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));
    
    if (ImGui::Button("REFRESH PROJECTS", ImVec2(-FLT_MIN, 45))) {
      refreshProjectList(m_SearchPath);
    }
    
    ImGui::EndChild(); // LeftColumnChild
    ImGui::PopID(); // LeftColumn
  }
  
  ImGui::NextColumn();
  
  {
    ImGui::PushID("RightColumn");
    ImGui::BeginChild("RightColumnChild", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar);
    
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(30, 20)); // Padding
    
    if (m_SelectedIndex != -1) {
      auto *proj = &m_FoundProjects[m_SelectedIndex];
      
      // Project title
      if (g_FontHeading)
        ImGui::PushFont(g_FontHeading);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.96f, 0.98f, 1.00f));
      ImGui::Text("%s", proj->getProjectName().c_str());
      ImGui::PopStyleColor();
      if (g_FontHeading)
        ImGui::PopFont();
      
      ImGui::Spacing();
      
      // Path
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.80f, 0.90f));
      ImGui::Text("📁 ");
      ImGui::SameLine();
      ImGui::TextWrapped("%s", proj->getPath().string().c_str());
      ImGui::PopStyleColor();
      
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
      
      // Details area with explicit size
      ImGui::BeginChild("DetailsChild", ImVec2(ImGui::GetContentRegionAvail().x - 30, 180), true);
      
      ImGui::Spacing();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.80f, 0.90f));
      ImGui::Text("PROJECT DETAILS");
      ImGui::PopStyleColor();
      
      ImGui::Spacing();
      
      ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12, 10));
      ImGui::PushID("ProjectDetailsTable");
      if (ImGui::BeginTable("ProjectDetailsTable", 2, 
                           ImGuiTableFlags_BordersInnerV | 
                           ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.80f, 0.90f));
        ImGui::Text("Engine Version");
        ImGui::PopStyleColor();
        
        ImGui::TableNextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.40f, 0.65f, 1.00f, 1.00f));
        ImGui::Text("%s", proj->getConfig().engineVersion.c_str());
        ImGui::PopStyleColor();
        
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.80f, 0.90f));
        ImGui::Text("Start Scene");
        ImGui::PopStyleColor();
        
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", proj->getConfig().startScenePath.c_str());
        
        ImGui::EndTable();
      }
      ImGui::PopID(); // ProjectDetailsTable
      ImGui::PopStyleVar();
      
      ImGui::EndChild(); // DetailsChild
      
      // Spacer before buttons
      ImGui::Dummy(ImVec2(0, 20));
      
      // Action buttons
      float buttonHeight = 50.0f;
      
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.40f, 0.75f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.50f, 0.90f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.35f, 0.65f, 1.00f));
      ImGui::PushID("OpenProjectButton");
      if (ImGui::Button("OPEN PROJECT", ImVec2(-FLT_MIN, buttonHeight))) {
        Engine::FileSystem::setProjectRoot(proj->getPath());
        projectOpened = proj;
      }
      ImGui::PopID();
      ImGui::PopStyleColor(3);
      
      // Two side-by-side buttons
      float availableWidth = ImGui::GetContentRegionAvail().x;
      float buttonSpacing = 15.0f;
      float buttonWidth = (availableWidth - buttonSpacing) / 2.0f;
      
      // Delete button
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.25f, 0.25f, 0.20f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.30f, 0.30f, 0.30f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.20f, 0.20f, 0.40f));
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.40f, 0.40f, 1.00f));
      ImGui::PushID("DeleteProjectButton");
      if (ImGui::Button("DELETE PROJECT", ImVec2(buttonWidth, 45))) {
        ImGui::OpenPopup("Confirm Delete?");
      }
      ImGui::PopID();
      ImGui::PopStyleColor(4);
      
      ImGui::SameLine();
      
      // Cancel button
      ImGui::PushID("CancelButton");
      if (ImGui::Button("CANCEL", ImVec2(buttonWidth, 45))) {
        m_SelectedIndex = -1;
      }
      ImGui::PopID();
      
    } else {
      // Create new project form
      static bool pathInitialized = false;
      if (!pathInitialized) {
        strcpy(m_NewProjectPath, m_SearchPath.c_str());
        pathInitialized = true;
      }
      
      ImGui::Spacing();
      if (g_FontHeading)
        ImGui::PushFont(g_FontHeading);
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.96f, 0.98f, 1.00f));
      ImGui::Text("Create New Project");
      ImGui::PopStyleColor();
      if (g_FontHeading)
        ImGui::PopFont();
      
      ImGui::Spacing();
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.80f, 0.90f));
      ImGui::Text("Start a new project from scratch. Choose a name and location.");
      ImGui::PopStyleColor();
      
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
      
      // Form area
      ImGui::BeginChild("CreateFormChild", ImVec2(ImGui::GetContentRegionAvail().x - 30, 250), true);
      
      ImGui::Spacing();
      ImGui::Dummy(ImVec2(20, 0));
      ImGui::SameLine();
      
      ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 12));
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 20));
      
      float formWidth = ImGui::GetContentRegionAvail().x - 20;
      
      // Project Name
      ImGui::Text("Project Name");
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1.00f));
      ImGui::SetNextItemWidth(formWidth);
      ImGui::InputText("##NameInput", m_NewProjectName, 256);
      ImGui::PopStyleColor();
      
      ImGui::Spacing();
      
      // Project Location
      ImGui::Text("Project Location");
      
      float browseButtonWidth = 100.0f;
      float pathInputWidth = formWidth - browseButtonWidth - 10;
      
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1.00f));
      ImGui::SetNextItemWidth(pathInputWidth);
      ImGui::InputText("##PathInput", m_NewProjectPath, 512);
      ImGui::PopStyleColor();
      
      ImGui::SameLine();
      ImGui::PushID("BrowseButton");
      if (ImGui::Button("Browse...", ImVec2(browseButtonWidth, ImGui::GetFrameHeight()))) {
        auto selection = pfd::select_folder("Select Project Directory").result();
        if (!selection.empty()) {
          std::strncpy(m_NewProjectPath, selection.c_str(), 255);
          m_NewProjectPath[255] = '\0';
        }
      }
      ImGui::PopID();
      
      ImGui::PopStyleVar(2);
      
      // Info text
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Spacing();
      
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.60f, 0.70f, 0.80f));
      ImGui::Text("ℹ️  The project folder will be created at the specified location.");
      ImGui::Spacing();
      ImGui::Text("ℹ️  A default project structure will be generated automatically.");
      ImGui::PopStyleColor();
      
      ImGui::EndChild(); // CreateFormChild
      
      // Spacer before button
      ImGui::Dummy(ImVec2(0, 20));
      
      // Create button
      float createButtonHeight = 55.0f;
      
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.55f, 0.30f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 0.35f, 1.00f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.25f, 1.00f));
      ImGui::PushID("CreateProjectButton");
      if (ImGui::Button("CREATE AND LAUNCH PROJECT", ImVec2(-FLT_MIN, createButtonHeight))) {
        if (strlen(m_NewProjectName) > 0)
          projectOpened = createNewProject();
      }
      ImGui::PopID();
      ImGui::PopStyleColor(3);
    }
    
    ImGui::EndGroup();
    ImGui::EndChild(); // RightColumnChild
    ImGui::PopID(); // RightColumn
  }
  
  ImGui::Columns(1); // Reset columns
  ImGui::EndChild(); // MainContentChild
  ImGui::PopID(); // MainContentArea
  
  ImGui::End(); // Main window

  if (g_FontDefault)
    ImGui::PopFont();

  ImGui::PopID(); // ProjectBrowserMainWindow

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

static fs::path FindProjectFile(const fs::path& projectRoot) {
    for (const auto& e : fs::directory_iterator(projectRoot)) {
        if (e.path().extension() == Engine::Project::Extension)
            return e.path();
    }
    return {};
}

Engine::Project* ProjectBrowser::createNewProject() {
    try {
        // 1. Project root: <selected-path>/<project-name>
        fs::path rootPath = fs::absolute(fs::path(m_NewProjectPath) / m_NewProjectName);

        if (fs::exists(rootPath)) {
            Engine::Log::error("Project folder already exists: " + rootPath.string());
            return nullptr;
        }

        fs::create_directories(rootPath);

        Engine::Project newProject;
        newProject.setName(m_NewProjectName);
        newProject.setPath(rootPath.string()); // absolute path!

        auto& config = newProject.getConfig();

        // 2. Create proper folders relative to root
        fs::create_directories(rootPath / config.assetDirectory / "textures");
        fs::create_directories(rootPath / config.assetDirectory / "scripts");
        fs::create_directories(rootPath / "scenes");

        // 3. Create .eproj in project root ONLY
        fs::path projectFile = rootPath / (m_NewProjectName + Engine::Project::Extension);
        Engine::ProjectSerializer::saveProjectFile(&newProject, projectFile);

        Engine::Log::info("Project created successfully at: " + rootPath.string());

        // 4. Update FileSystem
        Engine::FileSystem::setProjectRoot(rootPath.string());

        // 5. Refresh project list
        refreshProjectList(m_NewProjectPath);

        // 6. Return pointer from m_FoundProjects
        for (auto& project : m_FoundProjects) {
            if (project.getPath() == rootPath.string()) return &project;
        }

        return nullptr;

    } catch (const std::exception& e) {
        Engine::Log::error("Failed to create project: " + std::string(e.what()));
        return nullptr;
    }
}