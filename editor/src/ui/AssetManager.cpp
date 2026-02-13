#include "ecs/components/SpriteComponent.h"
#include "imgui.h"
#include "portable-file-dialogs.h"
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include "../EditorApp.h"
#include "core/Project.h"
#include "core/AssetManager.h"
#include "scene/Scene.h"
#include <fstream>
namespace fs = std::filesystem;
using namespace Engine;

void EditorApp::renderAssetManager()
{

    static fs::path itemToRename = "";
    static char renameBuffer[256] = "";
    static bool focusRenameInput = false;
    if (!m_currentProject)
    {
        ImGui::Begin("Asset Manager");
        ImGui::Text("No project loaded.");
        ImGui::End();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::Begin("Asset Manager", nullptr, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();

    fs::path projectRoot = m_currentProject->getPath();
    fs::path assetsRoot = projectRoot / "assets";

    static fs::path currentRelativePath = "";
    fs::path currentDirectoryPath = assetsRoot / currentRelativePath;

    if (!fs::exists(currentDirectoryPath))
    {
        fs::create_directories(currentDirectoryPath);
    }

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button("Assets"))
        currentRelativePath = "";

    fs::path accumulated;
    for (auto &part : currentRelativePath)
    {
        ImGui::SameLine();
        ImGui::TextDisabled(">");
        ImGui::SameLine();
        accumulated /= part;
        if (ImGui::Button(part.string().c_str()))
            currentRelativePath = accumulated;
    }
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginChild("AssetGrid", ImVec2(0, 0), false);

    if (ImGui::BeginPopupContextWindow("GridContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("New Folder"))
        {
            std::string folderName = "New Folder";
            fs::path folderPath = currentDirectoryPath / folderName;
            int counter = 1;
            while (fs::exists(folderPath))
            {
                folderName = "New Folder " + std::to_string(counter++);
                folderPath = currentDirectoryPath / folderName;
            }
            try {
                fs::create_directory(folderPath);
                Log::info("Created folder: " + folderName);
            } catch (const std::exception& e) {
                Log::error("Failed to create folder: " + std::string(e.what()));
            }
        }
        if (ImGui::MenuItem("Import Texture..."))
        {
            auto selection = pfd::open_file("Select Texture", ".", {"Images", "*.png *.jpg *.jpeg *.tga"}).result();
            if (!selection.empty())
            {
                fs::path sourcePath(selection[0]);
                fs::path destinationPath = currentDirectoryPath / sourcePath.filename();
                try
                {
                    fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing);
                    std::string assetId = (currentRelativePath / sourcePath.filename()).string();
                    m_AssetManager->loadTexture(assetId, destinationPath.string());
                }
                catch (const std::exception &e)
                {
                }
            }
        }
        if (ImGui::MenuItem("Import Audio..."))
        {
            auto selection = pfd::open_file("Select Audio", ".", {"Audio Files", "*.wav *.mp3 *.ogg"}).result();
            if (!selection.empty())
            {
                fs::path sourcePath(selection[0]);
                fs::path destinationPath = currentDirectoryPath / sourcePath.filename();
                try
                {
                    fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing);
                }
                catch (const std::exception &e)
                {
                }
            }
        }
        if (ImGui::MenuItem("New Lua Script"))
        {
            std::string filename = "NewScript.lua";
            fs::path scriptPath = currentDirectoryPath / filename;

            int counter = 1;
            while (fs::exists(scriptPath))
            {
                filename = "NewScript" + std::to_string(counter++) + ".lua";
                scriptPath = currentDirectoryPath / filename;
            }

            std::ofstream outfile(scriptPath);
            outfile << "-- " << filename << "\n\n"
                    << "function OnUpdate(dt)\n"
                    << "    -- code here\n"
                    << "end\n";
            outfile.close();

            Log::info("Created script: " + filename);
        }
        if (ImGui::MenuItem("Import Font..."))
        {
            auto selection = pfd::open_file("Select Font", ".", {"Font Files", "*.ttf"}).result();
            if (!selection.empty())
            {
                fs::path sourcePath(selection[0]);
                fs::path destinationPath = currentDirectoryPath / sourcePath.filename();
                try
                {
                    fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing);

                    // Register font in your AssetManager
                    std::string assetId = (currentRelativePath / sourcePath.filename()).string();
                    m_AssetManager->loadFontIfMissing(assetId, destinationPath.string(), 16, m_currentProject);

                    Log::info("Imported font: " + assetId);
                }
                catch (const std::exception &e)
                {
                    Log::error("Failed to import font: " + std::string(e.what()));
                }
            }
        }

        ImGui::EndPopup();
    }

    float thumbnailSize = 80.0f;
    float padding = 18.0f;
    float cellSize = thumbnailSize + padding;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / cellSize);
    if (columnCount < 1)
        columnCount = 1;

    ImGui::Columns(columnCount, nullptr, false);

    if (fs::exists(currentDirectoryPath))
    {
        for (auto const &entry : fs::directory_iterator(currentDirectoryPath))
        {
            const auto &path = entry.path();
            std::string filename = path.filename().string();
            if (filename.empty() || filename[0] == '.')
                continue;

            ImGui::PushID(filename.c_str());
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImDrawList *drawList = ImGui::GetWindowDrawList();

            ImGui::BeginGroup();

            bool isDir = entry.is_directory();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::InvisibleButton("##ItemBtn", ImVec2(thumbnailSize, thumbnailSize + 25));
            bool hovered = ImGui::IsItemHovered();
            bool active = ImGui::IsItemActive();
            ImGui::PopStyleVar();

            ImU32 slotColor = hovered ? IM_COL32(60, 60, 65, 255) : IM_COL32(40, 40, 42, 255);
            if (active)
                slotColor = IM_COL32(70, 70, 85, 255);
            drawList->AddRectFilled(pos, ImVec2(pos.x + thumbnailSize, pos.y + thumbnailSize), slotColor, 6.0f);

            if (isDir)
            {
                drawList->AddRectFilled(ImVec2(pos.x + 20, pos.y + 30), ImVec2(pos.x + 60, pos.y + 60), IM_COL32(230, 180, 50, 255), 2.0f);
                drawList->AddRectFilled(ImVec2(pos.x + 20, pos.y + 25), ImVec2(pos.x + 40, pos.y + 30), IM_COL32(230, 180, 50, 255), 2.0f);
                if (hovered && ImGui::IsMouseDoubleClicked(0))
                    currentRelativePath /= filename;
            }
            else
            {
                std::string ext = path.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                std::string assetId = (currentRelativePath / filename).string();

                if (ext == ".lua")
                {
                    ImVec2 center = ImVec2(pos.x + thumbnailSize * 0.5f, pos.y + thumbnailSize * 0.5f);
                    drawList->AddRect(ImVec2(center.x - 15, center.y - 20), ImVec2(center.x + 15, center.y + 20), IM_COL32(0, 255, 150, 255), 2.0f, 0, 2.0f);
                    drawList->AddText(ImVec2(center.x - 5, center.y - 10), IM_COL32(0, 255, 150, 255), "L");

                    if (hovered && ImGui::IsMouseDoubleClicked(0))
                    {
#ifdef _WIN32
                        std::string cmd = "start \"\" \"" + path.string() + "\"";
                        system(cmd.c_str());
#else
                        std::string cmd = "open \"" + path.string() + "\"";
                        system(cmd.c_str());
#endif
                    }
                }

                else if (ext == ".wav" || ext == ".mp3" || ext == ".ogg")
                {
                    ImVec2 center = ImVec2(pos.x + thumbnailSize * 0.5f, pos.y + thumbnailSize * 0.5f);
                    drawList->AddTriangleFilled(ImVec2(center.x - 10, center.y - 10), ImVec2(center.x - 10, center.y + 10), ImVec2(center.x + 5, center.y), IM_COL32(100, 200, 255, 255));
                    drawList->AddRectFilled(ImVec2(center.x - 18, center.y - 5), ImVec2(center.x - 10, center.y + 5), IM_COL32(100, 200, 255, 255));
                    drawList->AddCircle(center, 12.0f, IM_COL32(100, 200, 255, 150), 0, 1.5f);
                }

                else if (ext == ".ttf")
                {
                    ImVec2 center = ImVec2(pos.x + thumbnailSize * 0.5f, pos.y + thumbnailSize * 0.5f);
                    drawList->AddRect(ImVec2(center.x - 20, center.y - 20), ImVec2(center.x + 20, center.y + 20), IM_COL32(255, 200, 0, 255), 4.0f);
                    drawList->AddText(ImVec2(center.x - 10, center.y - 8), IM_COL32(255, 200, 0, 255), "F");

                    if (hovered && ImGui::IsMouseDoubleClicked(0))
                    {
#ifdef _WIN32
                        std::string cmd = "start " + path.string();
                        system(cmd.c_str());
#else
                        std::string cmd = "open " + path.string();
                        system(cmd.c_str());
#endif
                    }

                    if (ImGui::BeginDragDropSource())
                    {
                        std::string assetId = (currentRelativePath / filename).string();
                        ImGui::SetDragDropPayload("FONT_ASSET_ID", assetId.c_str(), assetId.size() + 1);
                        ImGui::Text("Dragging: %s", filename.c_str());
                        ImGui::EndDragDropSource();
                    }
                }

                else
                {
                    SDL_Texture *tex = m_AssetManager->getTexture(assetId);
                    if (tex)
                    {
                        drawList->AddImage((ImTextureID)tex, ImVec2(pos.x + 6, pos.y + 6), ImVec2(pos.x + thumbnailSize - 6, pos.y + thumbnailSize - 6));
                    }
                    else
                    {
                        drawList->AddText(ImVec2(pos.x + 25, pos.y + 35), IM_COL32(200, 200, 200, 255), ext.empty() ? "?" : ext.c_str());
                    }
                }

                if (ImGui::BeginDragDropSource())
                {
                    ImGui::SetDragDropPayload("ASSET_ID", assetId.c_str(), assetId.size() + 1);
                    ImGui::Text("Dragging: %s", filename.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            if (itemToRename == path)
            {
                std::string stem = path.stem().string();
                std::string ext = path.extension().string();

                if (focusRenameInput)
                {
                    strncpy(renameBuffer, stem.c_str(), sizeof(renameBuffer));
                    renameBuffer[sizeof(renameBuffer) - 1] = '\0';
                    ImGui::SetKeyboardFocusHere();
                    focusRenameInput = false;
                }

                ImGui::SetNextItemWidth(120.0f);
                bool renameDone = ImGui::InputText("##RenameInput", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::SameLine();
                ImGui::TextDisabled("%s", ext.c_str());

                if (renameDone)
                {
                    fs::path newPath = path.parent_path() / (std::string(renameBuffer) + ext);

                    if (!fs::exists(newPath))
                    {
                        try
                        {
                            fs::rename(path, newPath);
                            Log::info("Renamed: " + path.string() + " -> " + newPath.string());

                            std::string oldAssetId = fs::relative(path, m_currentProject->getPath() / "assets").string();
                            std::string newAssetId = fs::relative(newPath, m_currentProject->getPath() / "assets").string();

                            std::string extension = newPath.extension().string();
                            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga" || extension == ".bmp")
                            {
                                m_AssetManager->removeTexture(oldAssetId);
                                m_AssetManager->loadTextureIfMissing(newAssetId, newPath.string());
                            }
                        }
                        catch (const std::exception &e)
                        {
                            Log::error("Rename failed: " + std::string(e.what()));
                        }
                    }
                    itemToRename = "";
                }
            }
            else
            {
                std::string label = filename;
                if (label.length() > 11)
                    label = label.substr(0, 8) + "...";
                ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
                drawList->AddText(ImVec2(pos.x + (thumbnailSize - textSize.x) * 0.5f, pos.y + thumbnailSize + 4),
                                  IM_COL32(220, 220, 220, 255), label.c_str());
            }

            if (ImGui::BeginPopupContextItem("ItemCtx"))
            {
                if (ImGui::MenuItem("Rename"))
                {
                    itemToRename = path;
                    strncpy(renameBuffer, filename.c_str(), sizeof(renameBuffer));
                    focusRenameInput = true;
                }

                if (ImGui::MenuItem("Delete"))
                {
                    try
                    {
                        fs::remove_all(path);
                    }
                    catch (...)
                    {
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::EndGroup();
            ImGui::NextColumn();
            ImGui::PopID();
        }
    }

    ImGui::Columns(1);
    ImGui::EndChild();
    ImGui::End();
}

void EditorApp::loadProjectAssets()
{
    if (!m_currentProject || !m_AssetManager)
        return;

    fs::path assetsRoot = fs::path(m_currentProject->getPath()) / "assets";

    if (!fs::exists(assetsRoot))
    {
        fs::create_directories(assetsRoot);
        return;
    }

    for (auto const &entry : fs::recursive_directory_iterator(assetsRoot))
    {
        if (!entry.is_regular_file())
            continue;

        const auto &fullPath = entry.path();

        fs::path relativePath = fs::relative(fullPath, assetsRoot);
        std::string assetId = relativePath.string();
        std::string extension = fullPath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".tga" || extension == ".bmp")
        {
            if (!m_AssetManager->getTexture(assetId))
            {
                m_AssetManager->loadTexture(assetId, fullPath.string());
            }
        }
        else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg")
        {
        }
        else if (extension == ".ttf")
        {
            if (!m_AssetManager->getFont(assetId))
            {
                m_AssetManager->loadFontIfMissing(assetId, fullPath.string(), 16, m_currentProject);
                Log::info("Loaded font: " + assetId);
            }
        }
    }

    if (currentScene)
    {
        auto entities = currentScene->getEntityRawPointers();

        for (auto &entity : entities)
        {
            if (entity->hasComponent<Engine::SpriteComponent>())
            {
                auto sprite = entity->getComponent<Engine::SpriteComponent>();

                if (!sprite->assetId.empty())
                {
                    sprite->texture = m_AssetManager->getTexture(sprite->assetId);

                    if (!sprite->texture)
                    {
                        std::cout << "[Editor] Warning: Asset ID '" << sprite->assetId << "' not found for Entity: " << entity->getName() << std::endl;
                    }
                }
            }
        }
    }

    return;
}