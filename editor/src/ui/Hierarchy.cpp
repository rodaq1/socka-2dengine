#include "../EditorApp.h"
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <imgui_internal.h> // For specialized input focus logic

#include "core/ProjectSerializer.h"
#include "ecs/components/InheritanceComponent.h"

using namespace Engine;
namespace fs = std::filesystem;

static Entity *s_EntityPendingDeletion = nullptr;
static bool s_ShowDeleteModal = false;
static char s_RenameBuffer[128] = "";
static Entity *s_CopiedEntity = nullptr;

void GetHierarchyFlat(Entity *root, std::vector<Entity *> &outList)
{
    if (!root)
        return;
    outList.push_back(root);
    for (Entity *child : root->getChildren())
    {
        GetHierarchyFlat(child, outList);
    }
}

Entity *CloneEntityRecursive(Scene *scene, Entity *source, Entity *newParent = nullptr)
{
    if (!source)
        return nullptr;

    Entity *clone = scene->createEntity(source->getName() + " Copy");

    clone->copyAllComponentsFrom(source);

    if (newParent)
        clone->setParent(newParent);

    for (auto *child : source->getChildren())
    {
        CloneEntityRecursive(scene, child, clone);
    }

    return clone;
}

void DrawEntityNode(Entity *entity, Entity *&selectedEntity, std::vector<Entity *> &pendingDeletion, Entity *&entityToRename)
{
    if (!entity || entity->isRemoved)
        return;

    ImGui::PushID(entity);

    const auto &children = entity->getChildren();
    ImGuiTreeNodeFlags flags = ((selectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0);
    flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    else
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    bool isRenaming = (entityToRename == entity);
    bool opened = false;

    if (isRenaming)
    {
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputText("##rename", s_RenameBuffer, sizeof(s_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            if (strlen(s_RenameBuffer) > 0)
                entity->setName(s_RenameBuffer);
            entityToRename = nullptr;
        }
        if (!ImGui::IsItemActive() && ImGui::IsMouseClicked(0))
            entityToRename = nullptr;
    }
    else
    {
        opened = ImGui::TreeNodeEx((void *)(uintptr_t)entity->getHandle(), flags, "%s", entity->getName().c_str());

        if (ImGui::IsItemClicked())
        {
            selectedEntity = entity;
        }

        // Double click to rename
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            entityToRename = entity;
            strncpy(s_RenameBuffer, entity->getName().c_str(), sizeof(s_RenameBuffer));
        }
    }

    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("ENTITY_HIERARCHY_ITEM", &entity, sizeof(Entity *));
        ImGui::Text("Moving %s", entity->getName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_HIERARCHY_ITEM"))
        {
            Entity *draggedEntity = *(Entity **)payload->Data;
            if (draggedEntity != entity)
            {
                draggedEntity->setParent(entity);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Context Menu
    if (ImGui::BeginPopupContextItem())
    {
        selectedEntity = entity;
        if (ImGui::MenuItem("Rename", "F2"))
        {
            entityToRename = entity;
            strncpy(s_RenameBuffer, entity->getName().c_str(), sizeof(s_RenameBuffer));
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete Entity"))
        {
            if (!entity->getChildren().empty())
            {
                s_EntityPendingDeletion = entity;
                s_ShowDeleteModal = true;
            }
            else
            {
                pendingDeletion.push_back(entity);
            }
        }
        ImGui::EndPopup();
    }

    if (!(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen) && opened)
    {
        for (Entity *child : children)
        {
            DrawEntityNode(child, selectedEntity, pendingDeletion, entityToRename);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void EditorApp::renderHierarchy()
{
    if (m_AppState != AppState::EDITOR || !m_currentProject || !currentScene)
        return;
    ImGui::Begin("Hierarchy", &m_ShowHierarchy);

    if (!m_currentProject)
    {
        ImGui::TextDisabled("No project loaded.");
        ImGui::End();
        return;
    }

    static std::string sceneToProcessPath = "";
    static std::string sceneToLoadPath = "";
    static bool s_OpenRenameScenePopup = false;
    static bool s_OpenDeleteScenePopup = false;
    static bool s_OpenSaveConfirmPopup = false;

    if (ImGui::CollapsingHeader("Scenes", ImGuiTreeNodeFlags_DefaultOpen))
    {
        fs::path sceneDir = m_currentProject->getPath() / "scenes";
        if (!fs::exists(sceneDir))
            fs::create_directories(sceneDir);

        if (ImGui::Button("+ New Scene", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
            ImGui::OpenPopup("CreateNewScenePopup");
        }

        ImGui::Separator();

        for (auto &entry : fs::directory_iterator(sceneDir))
        {
            if (entry.path().extension() == ".json")
            {
                std::string pathStr = entry.path().string();
                std::string stem = entry.path().stem().string();
                bool isActive = (activeScenePath == pathStr);

                ImGui::PushID(pathStr.c_str());

                bool isVisualSelected = isActive && m_SceneSelected;
                if (isVisualSelected)
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));
                if (isActive)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));

                if (ImGui::Selectable(stem.c_str(), isVisualSelected))
                {
                    if (!isActive)
                    {
                        sceneToLoadPath = pathStr;
                        if (currentScene)
                        {
                            s_OpenSaveConfirmPopup = true;
                        }
                        else
                        {
                            m_SelectedEntity = nullptr;
                            ProjectSerializer::loadScene(currentScene, m_Renderer, sceneToLoadPath, m_AssetManager.get(), m_currentProject);
                            setActiveScene(sceneToLoadPath);
                            activeScenePath = sceneToLoadPath;
                        }
                    }
                    m_SceneSelected = true;
                    m_SelectedEntity = nullptr;
                }

                if (ImGui::BeginPopupContextItem("SceneContext"))
                {
                    if (isActive)
                    {
                        if (ImGui::MenuItem("Rename Scene"))
                        {
                            sceneToProcessPath = pathStr;
                            strncpy(s_RenameBuffer, stem.c_str(), sizeof(s_RenameBuffer));
                            s_OpenRenameScenePopup = true;
                        }
                        if (ImGui::MenuItem("Delete Scene"))
                        {
                            sceneToProcessPath = pathStr;
                            s_OpenDeleteScenePopup = true;
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("Load scene to modify");
                        if (ImGui::MenuItem("Load Scene"))
                        {
                            sceneToLoadPath = pathStr;
                            if (currentScene)
                                s_OpenSaveConfirmPopup = true;
                            else
                            {
                                m_SelectedEntity = nullptr;
                                ProjectSerializer::loadScene(currentScene, m_Renderer, sceneToLoadPath, m_AssetManager.get(), m_currentProject);
                                setActiveScene(sceneToLoadPath);
                                activeScenePath = sceneToLoadPath;
                            }
                        }
                    }
                    ImGui::EndPopup();
                }

                if (isActive)
                    ImGui::PopStyleColor();
                if (isVisualSelected)
                    ImGui::PopStyleColor();
                ImGui::PopID();
            }
        }
    }

    // --- Save Confirmation Modal ---
    if (s_OpenSaveConfirmPopup)
    {
        ImGui::OpenPopup("Save Changes?");
        s_OpenSaveConfirmPopup = false;
    }

    if (ImGui::BeginPopupModal("Save Changes?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("You are about to switch scenes.\nDo you want to save changes to the current scene first?");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save & Switch", ImVec2(120, 0)))
        {
            if (currentScene && !activeScenePath.empty())
            {
                Engine::ProjectSerializer::saveScene(currentScene.get(), activeScenePath);
            }
            m_SelectedEntity = nullptr;
            ProjectSerializer::loadScene(currentScene, m_Renderer, sceneToLoadPath, m_AssetManager.get(), m_currentProject);
            setActiveScene(sceneToLoadPath);
            activeScenePath = sceneToLoadPath;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard", ImVec2(120, 0)))
        {
            m_SelectedEntity = nullptr;
            ProjectSerializer::loadScene(currentScene, m_Renderer, sceneToLoadPath, m_AssetManager.get(), m_currentProject);
            setActiveScene(sceneToLoadPath);
            activeScenePath = sceneToLoadPath;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // --- Scene Renaming Modal ---
    if (s_OpenRenameScenePopup)
    {
        ImGui::OpenPopup("RenameScenePopup");
        s_OpenRenameScenePopup = false;
    }

    if (ImGui::BeginPopupModal("RenameScenePopup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter new name for scene:");
        ImGui::InputText("##newName", s_RenameBuffer, sizeof(s_RenameBuffer));

        ImGui::Spacing();
        if (ImGui::Button("Apply", ImVec2(120, 0)))
        {
            fs::path oldPath(sceneToProcessPath);
            std::string newNameStr = std::string(s_RenameBuffer);
            fs::path newPath = oldPath.parent_path() / (newNameStr + ".json");

            if (strlen(s_RenameBuffer) > 0 && !fs::exists(newPath))
            {
                bool wasActive = (activeScenePath == sceneToProcessPath);
                try
                {
                    fs::rename(oldPath, newPath);

                    if (wasActive)
                    {
                        activeScenePath = newPath.string();

                        if (currentScene)
                        {
                            currentScene->setName(newNameStr);
                            Engine::ProjectSerializer::saveScene(currentScene.get(), activeScenePath);
                        }
                    }
                }
                catch (const fs::filesystem_error &e)
                {
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // --- Scene Deletion Modal ---
    if (s_OpenDeleteScenePopup)
    {
        ImGui::OpenPopup("DeleteScenePopup");
        s_OpenDeleteScenePopup = false;
    }

    if (ImGui::BeginPopupModal("DeleteScenePopup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to delete this scene?\nThis action cannot be undone.");
        ImGui::TextDisabled("%s", sceneToProcessPath.c_str());

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Delete", ImVec2(120, 0)))
        {
            try
            {
                if (fs::exists(sceneToProcessPath))
                {
                    bool wasActive = (activeScenePath == sceneToProcessPath);
                    fs::remove(sceneToProcessPath);
                    if (wasActive)
                    {
                        activeScenePath = "";
                        currentScene = nullptr;
                        m_SelectedEntity = nullptr;
                        m_SceneSelected = false;
                    }
                }
            }
            catch (const fs::filesystem_error &)
            {
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!currentScene)
    {
        ImGui::TextDisabled("No active scene.");
        ImGui::End();
        return;
    }

    if (ImGui::Button("+ Create Entity", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        m_SelectedEntity = currentScene->createEntity("New Entity");
        m_SceneSelected = false;
    }

    std::vector<Entity *> pendingDeletion;
    static Entity *entityToRename = nullptr;

    // --- Entity Tree ---
    if (ImGui::BeginChild("EntityTree"))
    {
        // Global shortcut for F2 renaming
        if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F2) && m_SelectedEntity)
        {
            entityToRename = m_SelectedEntity;
            strncpy(s_RenameBuffer, m_SelectedEntity->getName().c_str(), sizeof(s_RenameBuffer));
        }

        ImGuiIO &io = ImGui::GetIO();

        if (ImGui::IsWindowFocused())
        {
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D) && m_SelectedEntity)
            {
                m_SelectedEntity =
                    CloneEntityRecursive(currentScene.get(), m_SelectedEntity,
                                         m_SelectedEntity->getParent());

                m_SceneSelected = false;
            }
        }
        for (Entity *entity : currentScene->getEntityRawPointers())
        {
            if (!entity || entity->isRemoved)
                continue;

            if (entity->getParent() == nullptr)
            {
                DrawEntityNode(entity, m_SelectedEntity, pendingDeletion, entityToRename);
                if (ImGui::IsItemClicked())
                    m_SceneSelected = false;
            }
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ENTITY_HIERARCHY_ITEM"))
            {
                Entity *draggedEntity = *(Entity **)payload->Data;
                draggedEntity->setParent(nullptr);
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::EndChild();

    // --- Entity Deletion Modal ---
    if (s_ShowDeleteModal)
    {
        ImGui::OpenPopup("Delete Hierarchy?");
        s_ShowDeleteModal = false;
    }

    if (ImGui::BeginPopupModal("Delete Hierarchy?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("The entity '%s' has children.\nWhat would you like to do?",
                    s_EntityPendingDeletion ? s_EntityPendingDeletion->getName().c_str() : "Unknown");
        ImGui::Separator();

        if (ImGui::Button("Delete All", ImVec2(120, 0)))
        {
            if (s_EntityPendingDeletion)
            {
                GetHierarchyFlat(s_EntityPendingDeletion, pendingDeletion);
            }
            s_EntityPendingDeletion = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("Keep Children (Orphan)", ImVec2(160, 0)))
        {
            if (s_EntityPendingDeletion)
            {
                auto children = s_EntityPendingDeletion->getChildren();
                for (auto *child : children)
                {
                    child->setParent(nullptr);
                }
                pendingDeletion.push_back(s_EntityPendingDeletion);
            }
            s_EntityPendingDeletion = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            s_EntityPendingDeletion = nullptr;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    for (Entity *e : pendingDeletion)
    {
        if (m_SelectedEntity == e)
            m_SelectedEntity = nullptr;
        if (e && !e->isRemoved)
        {
            currentScene->destroyEntity(e);
        }
    }

    ImGui::End();
}