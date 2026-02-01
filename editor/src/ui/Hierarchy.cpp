#include "../EditorApp.h"
#include <filesystem>
#include <fstream>
#include <imgui.h>

#include "core/ProjectSerializer.h"
#include "ecs/components/InheritanceComponent.h" 

using namespace Engine;
namespace fs = std::filesystem;

static Entity* s_EntityPendingDeletion = nullptr;
static bool s_ShowDeleteModal = false;

void GetHierarchyFlat(Entity* root, std::vector<Entity*>& outList) {
    if (!root) return;
    outList.push_back(root);
    for (Entity* child : root->getChildren()) {
        GetHierarchyFlat(child, outList);
    }
}

void DrawEntityNode(Entity* entity, Entity*& selectedEntity, std::vector<Entity*>& pendingDeletion) {
    if (!entity || entity->isRemoved) return;

    ImGui::PushID(entity);

    const auto& children = entity->getChildren();
    ImGuiTreeNodeFlags flags = ((selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
    flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    
    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    } else {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)entity->getHandle(), flags, "%s", entity->getName().c_str());
    
    if (ImGui::IsItemClicked()) {
        selectedEntity = entity;
    }

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("ENTITY_HIERARCHY_ITEM", &entity, sizeof(Entity*));
        ImGui::Text("Moving %s", entity->getName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_HIERARCHY_ITEM")) {
            Entity* draggedEntity = *(Entity**)payload->Data;
            if (draggedEntity != entity) {
                draggedEntity->setParent(entity);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Context Menu
    if (ImGui::BeginPopupContextItem()) {
        selectedEntity = entity;
        if (ImGui::MenuItem("Delete Entity")) {
            // Check if entity has children to determine if we show a popup or just delete
            if (!entity->getChildren().empty()) {
                s_EntityPendingDeletion = entity;
                s_ShowDeleteModal = true;
            } else {
                pendingDeletion.push_back(entity);
            }
        }
        ImGui::EndPopup();
    }

    if (!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen) && opened) {
        for (Entity* child : children) {
            DrawEntityNode(child, selectedEntity, pendingDeletion);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void EditorApp::renderHierarchy() {
    ImGui::Begin("Hierarchy", &m_ShowHierarchy);

    if (!m_currentProject) {
        ImGui::TextDisabled("No project loaded.");
        ImGui::End();
        return;
    }

    // --- Scene Header Logic ---
    if (ImGui::CollapsingHeader("Scenes", ImGuiTreeNodeFlags_DefaultOpen)) {
        fs::path sceneDir = m_currentProject->getPath() / "scenes";
        if (!fs::exists(sceneDir)) fs::create_directories(sceneDir);

        if (ImGui::Button("+ New Scene", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
            ImGui::OpenPopup("CreateNewScenePopup");
        }

        if (ImGui::BeginPopup("CreateNewScenePopup")) {
            static char newSceneName[64] = "NewLevel";
            ImGui::InputText("Name", newSceneName, sizeof(newSceneName));
            if (ImGui::Button("Create File")) {
                std::string filename = std::string(newSceneName) + ".json";
                fs::path fullPath = sceneDir / filename;
                std::ofstream ofs(fullPath);
                ofs << "{\"SceneName\":\"" << newSceneName << "\",\"Entities\":[]}";
                ofs.close();
                m_SelectedEntity = nullptr;
                ProjectSerializer::loadScene(currentScene, m_Renderer, fullPath.string(), m_AssetManager.get());
                activeScenePath = fullPath.string();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        for (auto& entry : fs::directory_iterator(sceneDir)) {
            if (entry.path().extension() == ".json") {
                std::string pathStr = entry.path().string();
                bool isActive = (activeScenePath == pathStr);
                
                ImGui::PushID(pathStr.c_str());
                
                bool isVisualSelected = isActive && m_SceneSelected;
                if (isVisualSelected) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
                if (isActive) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                
                if (ImGui::Selectable(entry.path().stem().string().c_str(), isVisualSelected)) {
                    if (!isActive) {
                        m_SelectedEntity = nullptr;
                        ProjectSerializer::loadScene(currentScene, m_Renderer, pathStr, m_AssetManager.get());
                        setActiveScene(pathStr);
                        activeScenePath = pathStr;
                    }
                    m_SceneSelected = true;
                    m_SelectedEntity = nullptr; 
                }
                
                if (isActive) ImGui::PopStyleColor();
                if (isVisualSelected) ImGui::PopStyleColor();
                ImGui::PopID();
            }
        }
    }

    ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

    if (!currentScene) {
        ImGui::TextDisabled("No active scene.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("+ Create Entity", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
        m_SelectedEntity = currentScene->createEntity("New Entity");
        m_SceneSelected = false;
    }

    std::vector<Entity*> pendingDeletion;

    // --- Entity Tree ---
    if (ImGui::BeginChild("EntityTree")) {
        for (Entity* entity : currentScene->getEntityRawPointers()) {
            if (!entity || entity->isRemoved) continue;
            
            if (entity->getParent() == nullptr) {
                DrawEntityNode(entity, m_SelectedEntity, pendingDeletion);
                if (ImGui::IsItemClicked()) m_SceneSelected = false; 
            }
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_HIERARCHY_ITEM")) {
                Entity* draggedEntity = *(Entity**)payload->Data;
                draggedEntity->setParent(nullptr);
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::EndChild();

    // --- Deletion Modal ---
    if (s_ShowDeleteModal) {
        ImGui::OpenPopup("Delete Hierarchy?");
        s_ShowDeleteModal = false;
    }

    if (ImGui::BeginPopupModal("Delete Hierarchy?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("The entity '%s' has children.\nWhat would you like to do?", 
                    s_EntityPendingDeletion ? s_EntityPendingDeletion->getName().c_str() : "Unknown");
        ImGui::Separator();

        if (ImGui::Button("Delete All", ImVec2(120, 0))) {
            if (s_EntityPendingDeletion) {
                // EXPLICIT RECURSION: Add parent and all nested children to deletion list
                GetHierarchyFlat(s_EntityPendingDeletion, pendingDeletion);
            }
            s_EntityPendingDeletion = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Keep Children (Orphan)", ImVec2(160, 0))) {
            if (s_EntityPendingDeletion) {
                // Detach children so they become root entities
                auto children = s_EntityPendingDeletion->getChildren();
                for (auto* child : children) {
                    child->setParent(nullptr);
                }
                pendingDeletion.push_back(s_EntityPendingDeletion);
            }
            s_EntityPendingDeletion = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            s_EntityPendingDeletion = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Execution of deletions
    for (Entity* e : pendingDeletion) {
        if (m_SelectedEntity == e) m_SelectedEntity = nullptr;
        // Double check validity as a child might have already been destroyed via recursion
        if (e && !e->isRemoved) {
            currentScene->destroyEntity(e);
        }
    }

    ImGui::End();
}