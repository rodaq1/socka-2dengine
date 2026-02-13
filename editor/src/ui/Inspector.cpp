#include "../EditorApp.h"
#include "core/AssetManager.h"
#include "ecs/Entity.h"
#include "ecs/components/AnimationComponent.h"
#include "ecs/components/BoxColliderComponent.h"
#include "ecs/components/CameraComponent.h"
#include "ecs/components/CircleColliderComponent.h"
#include "ecs/components/InputControllerComponent.h"
#include "ecs/components/PolygonColliderComponent.h"
#include "ecs/components/RigidBodyComponent.h"
#include "ecs/components/ScriptComponent.h"
#include "ecs/components/SoundComponent.h"
#include "ecs/components/SpriteComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/VelocityComponent.h"
#include "ecs/systems/ScriptSystem.h"
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"
#include "scene/Scene.h"
#include <cstring>

#include "../EditorApp.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/InheritanceComponent.h"
#include "ecs/components/TextComponent.h"
#include "imgui.h"
#include "core/FontManager.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <algorithm>

void EditorApp::renderInspector()
{
  ImGui::Begin("Inspector", &m_ShowInspector);
  ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);

  if (ImGui::SmallButton("?##InspectorInfo"))
  {
    ImGui::OpenPopup("InspectorInfoPopup");
  }

  ImGui::SetNextWindowSize(ImVec2(320.0f, 0.0f), ImGuiCond_Always);

  if (ImGui::BeginPopup("InspectorInfoPopup"))
  {
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 300.0f);

    ImGui::Text("Inspector Overview");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(
        "The Inspector lets you edit the currently selected "
        "entity or scene in real-time.");

    ImGui::Spacing();
    ImGui::BulletText("Modify components");
    ImGui::BulletText("Adjust transforms");
    ImGui::BulletText("Configure physics & rendering");
    ImGui::BulletText("Manage scripts & input");

    ImGui::Spacing();
    ImGui::TextWrapped("All changes are applied immediately.");

    ImGui::PopTextWrapPos();
    ImGui::EndPopup();
  }
  auto EndInspector = []()
  {
    ImGui::End();
  };

  if (m_AppState != AppState::EDITOR || !m_currentProject || !currentScene)
    return;

  if (m_SceneSelected && currentScene)
  {
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Scene Settings: %s", currentScene->getName().c_str());
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Background"))
    {
      auto &bg = currentScene->getBackground();

      const char *bgTypes[] = {"Solid Color", "Vertical Gradient", "Image"};
      int currentType = (int)bg.type;
      if (ImGui::Combo("Type", &currentType, bgTypes, 3))
      {
        bg.type = (Engine::BackgroundType)currentType;
      }

      if (bg.type == Engine::BackgroundType::Solid)
      {
        ImGui::ColorEdit3("Color", &bg.color1.x);
      }
      else if (bg.type == Engine::BackgroundType::Gradient)
      {
        ImGui::ColorEdit3("Top Color", &bg.color1.x);
        ImGui::ColorEdit3("Bottom Color", &bg.color2.x);
      }
      else if (bg.type == Engine::BackgroundType::Image)
      {
        ImGui::Text("Current Asset: %s", bg.assetId.empty() ? "None" : bg.assetId.c_str());
        if (ImGui::Button("Select Image..."))
        {
          ImGui::OpenPopup("BgImagePopup");
        }

        if (ImGui::BeginPopup("BgImagePopup"))
        {
          for (auto const &[id, tex] : Engine::AssetManager::m_Textures)
          {
            if (ImGui::Selectable(id.c_str(), bg.assetId == id))
            {
              bg.assetId = id;
            }
          }
          ImGui::EndPopup();
        }
        ImGui::Checkbox("Stretch to Fill", &bg.stretch);
      }
    }

    if (ImGui::CollapsingHeader("Active Camera"))
    {
      auto *camera = currentScene->getSceneCamera();
      if (camera)
      {
        // --- POSITION ---
        glm::vec2 camPos = camera->getPosition();
        if (ImGui::DragFloat2("Position", &camPos.x, 0.5f))
        {
          camera->setPosition(camPos);
        }

        ImGui::Separator();

        // --- ORTHO SIZE (ZOOM) ---
        float size = camera->getOrthographicSize();

        // Add a dropdown for common zoom levels
        const char *sizeLabels[] = {"Custom", "Close (5.0)", "Standard (10.0)", "Wide (20.0)", "Massive (50.0)"};
        float sizeValues[] = {size, 5.0f, 10.0f, 20.0f, 50.0f};
        static int currentSizeIdx = 0; // Note: In a real app, logic to match current 'size' to an index is better

        if (ImGui::Combo("Size Presets", &currentSizeIdx, sizeLabels, IM_ARRAYSIZE(sizeLabels)))
        {
          if (currentSizeIdx > 0)
          {
            size = sizeValues[currentSizeIdx];
            camera->setOrthographicSize(size);
          }
        }

        if (ImGui::DragFloat("Ortho Size", &size, 0.1f, 0.1f, 2000.0f))
        {
          camera->setOrthographicSize(size);
          currentSizeIdx = 0; // Set to 'Custom' if dragged manually
        }

        ImGui::Separator();

        // --- ASPECT RATIO ---
        float aspect = camera->getAspectRatio();

        struct AspectPreset
        {
          const char *Name;
          float Ratio;
        };
        static AspectPreset aspectPresets[] = {
            {"Custom", 0.0f},
            {"16:9 (Widescreen)", 16.0f / 9.0f},
            {"21:9 (Ultrawide)", 21.0f / 9.0f},
            {"4:3 (Classic)", 4.0f / 3.0f},
            {"1:1 (Square)", 1.0f / 1.0f},
            {"9:16 (Portrait)", 9.0f / 16.0f}};
        static int currentAspectIdx = 0;

        if (ImGui::Combo("Aspect Presets", &currentAspectIdx, [](void *data, int idx, const char **out_text)
                         {
            *out_text = aspectPresets[idx].Name;
            return true; }, nullptr, IM_ARRAYSIZE(aspectPresets)))
        {
          if (currentAspectIdx > 0)
          {
            aspect = aspectPresets[currentAspectIdx].Ratio;
            camera->setAspectRatio(aspect);
          }
        }

        if (ImGui::DragFloat("Aspect Ratio", &aspect, 0.01f, 0.1f, 10.0f))
        {
          camera->setAspectRatio(aspect);
          currentAspectIdx = 0; // Set to 'Custom'
        }

        ImGui::Separator();

        // --- CLIPPING PLANES ---
        float nearClip = camera->getNearClip();
        float farClip = camera->getFarClip();

        if (ImGui::DragFloat("Near Clip", &nearClip, 0.1f))
        {
          camera->setNearClip(nearClip);
        }
        if (ImGui::DragFloat("Far Clip", &farClip, 0.1f))
        {
          camera->setFarClip(farClip);
        }

        if (ImGui::Button("Reset Camera", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
        {
          camera->setPosition({0.0f, 0.0f});
          camera->setRotation(0.0f);
          camera->setOrthographicSize(10.0f);
          camera->setAspectRatio(16.0f / 9.0f);
          currentSizeIdx = 2;   // Standard
          currentAspectIdx = 1; // 16:9
        }
      }
      else
      {
        ImGui::TextDisabled("No camera found in scene.");
      }
    }

    EndInspector();
    return;
  }

  if (!m_SelectedEntity)
  {
    ImGui::TextDisabled("No entity or scene selected.");
    EndInspector();
    return;
  }
  char nameBuffer[256];
  strncpy(nameBuffer, m_SelectedEntity->getName().c_str(), sizeof(nameBuffer));
  if (ImGui::InputText("##EntityName", nameBuffer, sizeof(nameBuffer)))
  {
    m_SelectedEntity->setName(nameBuffer);
  }

  if (auto *parent = m_SelectedEntity->getParent())
  {
    ImGui::SameLine();
    ImGui::TextDisabled("(Child of %s)", parent->getName().c_str());
  }

  ImGui::Separator();

  if (m_SelectedEntity->hasComponent<Engine::TransformComponent>())
  {
    if (ImGui::CollapsingHeader("Transform"))
    {
      ImGui::PushID("Transform");
      auto tr = m_SelectedEntity->getComponent<Engine::TransformComponent>();

      const char *posLabel = m_SelectedEntity->getParent() ? "Local Position" : "Position";
      const char *rotLabel = m_SelectedEntity->getParent() ? "Local Rotation" : "Rotation";
      const char *scaleLabel = m_SelectedEntity->getParent() ? "Local Scale" : "Scale";

      ImGui::DragFloat2(posLabel, &tr->position.x, 1.0f);

      float rotation = tr->rotation;
      if (ImGui::DragFloat(rotLabel, &rotation, 0.5f))
      {
        tr->rotation = rotation;
      }

      ImGui::DragFloat2(scaleLabel, &tr->scale.x, 0.1f, 0.001f, 100.0f);

      // Show Read-Only World Position for convenience
      if (m_SelectedEntity->getParent())
      {
        ImGui::BeginDisabled();
        glm::vec2 worldPos = m_SelectedEntity->getWorldPosition(); // See implementation below
        ImGui::DragFloat2("World Position (Read-Only)", &worldPos.x);
        ImGui::EndDisabled();
      }
      ImGui::PopID();
    }
  }

  if (m_SelectedEntity->hasComponent<Engine::SpriteComponent>())
  {
    auto sprite = m_SelectedEntity->getComponent<Engine::SpriteComponent>();

    if (ImGui::CollapsingHeader("Sprite"))
    {
      ImGui::PushID("Sprite");
      if (ImGui::BeginPopupContextItem("SpriteContext"))
      {
        if (ImGui::MenuItem("Remove Sprite Component"))
        {
          m_SelectedEntity->removeComponent<Engine::SpriteComponent>();
        }
        ImGui::EndPopup();
      }
      ImGui::Checkbox("Visible", &sprite->visible);
      ImGui::Separator();

      ImGui::Text("Asset ID");
      ImGui::PushItemWidth(-1);

      ImGui::Text("Current: %s",
                  sprite->assetId.empty() ? "None" : sprite->assetId.c_str());

      if (ImGui::Button("Change Texture...",
                        ImVec2(ImGui::GetContentRegionAvail().x, 0)))
      {
        ImGui::OpenPopup("AssetSelectorPopup");
      }

      if (ImGui::BeginPopup("AssetSelectorPopup"))
      {
        static char assetSearch[64] = "";
        ImGui::InputTextWithHint("##AssetSearch", "Search...", assetSearch,
                                 sizeof(assetSearch));
        ImGui::Separator();

        ImGui::BeginChild("AssetList", ImVec2(250, 200));
        for (auto const &[assetId, tex] : Engine::AssetManager::m_Textures)
        {
          if (strlen(assetSearch) > 0)
          {
            if (assetId.find(assetSearch) == std::string::npos)
              continue;
          }

          if (ImGui::Selectable(assetId.c_str(), sprite->assetId == assetId))
          {
            sprite->assetId = assetId;
            sprite->texture = Engine::AssetManager::getTexture(sprite->assetId);

            if (sprite->sourceRect.w == 0 || sprite->sourceRect.h == 0)
            {
              int w, h;
              SDL_QueryTexture(tex, NULL, NULL, &w, &h);
              sprite->sourceRect = {0, 0, w, h};
            }
            ImGui::CloseCurrentPopup();
          }
        }
        ImGui::EndChild();
        ImGui::EndPopup();
      }
      ImGui::PopItemWidth();

      if (ImGui::Button("Manual Reload", ImVec2(-1, 0)))
      {
        sprite->texture = Engine::AssetManager::getTexture(sprite->assetId);
      }

      ImGui::Separator();
      ImGui::Text("Source Rect (First Frame)");
      ImGui::DragInt4("XYWH", &sprite->sourceRect.x, 1.0f, 0);

      ImGui::Separator();
      ImGui::Checkbox("Fixed To Screen", &sprite->isFixed);
      ImGui::DragInt("Z Index", &sprite->zIndex, 1.0f);

      ImGui::Separator();
      float color[4] = {sprite->color.r / 255.0f, sprite->color.g / 255.0f,
                        sprite->color.b / 255.0f, sprite->color.a / 255.0f};

      if (ImGui::ColorEdit4("Tint", color))
      {
        sprite->color.r = (Uint8)(color[0] * 255);
        sprite->color.g = (Uint8)(color[1] * 255);
        sprite->color.b = (Uint8)(color[2] * 255);
        sprite->color.a = (Uint8)(color[3] * 255);
      }

      ImGui::Checkbox("Flip Horizontal", &sprite->flipH);
      ImGui::Checkbox("Flip Vertical", &sprite->flipV);

      ImGui::Separator();

      if (sprite->texture)
      {
        ImGui::Text("Preview:");

        int texW, texH;
        SDL_QueryTexture(sprite->texture, NULL, NULL, &texW, &texH);

        ImVec2 uv0 = ImVec2((float)sprite->sourceRect.x / texW,
                            (float)sprite->sourceRect.y / texH);
        ImVec2 uv1 =
            ImVec2((float)(sprite->sourceRect.x + sprite->sourceRect.w) / texW,
                   (float)(sprite->sourceRect.y + sprite->sourceRect.h) / texH);

        ImVec2 availSize = ImGui::GetContentRegionAvail();
        float aspect =
            (sprite->sourceRect.h > 0)
                ? (float)sprite->sourceRect.w / (float)sprite->sourceRect.h
                : 1.0f;
        ImVec2 drawSize = {availSize.x, availSize.x / aspect};

        ImVec4 tintColor =
            ImVec4(sprite->color.r / 255.0f, sprite->color.g / 255.0f,
                   sprite->color.b / 255.0f, sprite->color.a / 255.0f);

        ImGui::Image((ImTextureID)sprite->texture, drawSize, uv0, uv1,
                     tintColor, ImVec4(1, 1, 1, 0.1f));
      }
      else
      {
        ImGui::TextDisabled("No texture loaded");
      }

      ImGui::Separator();
      if (ImGui::Button("Remove Sprite Component", ImVec2(-1, 0)))
      {
        m_SelectedEntity->removeComponent<Engine::SpriteComponent>();
      }
      ImGui::PopID();
    }
  }

  if (m_SelectedEntity->hasComponent<Engine::AnimationComponent>())
  {
    auto anim = m_SelectedEntity->getComponent<Engine::AnimationComponent>();
    if (ImGui::CollapsingHeader("Animation"))
    {
      ImGui::PushID("Animation");
      if (ImGui::BeginPopupContextItem("AnimationContext"))
      {
        if (ImGui::MenuItem("Remove Animation Component"))
        {
          m_SelectedEntity->removeComponent<Engine::AnimationComponent>();
        }
        ImGui::EndPopup();
      }

      // Playback Controls
      float buttonWidth = ImGui::GetContentRegionAvail().x * 0.5f - 4.0f;
      if (anim->isPlaying)
      {
        if (ImGui::Button("Stop", ImVec2(buttonWidth, 0)))
          anim->stop();
      }
      else
      {
        if (ImGui::Button("Play", ImVec2(buttonWidth, 0)))
          anim->play(anim->currentAnimationName);
      }
      ImGui::SameLine();
      ImGui::Checkbox("Looping", &anim->isLooping);

      ImGui::Separator();

      // Animation Selector
      ImGui::Text("Current State:");
      ImGui::PushItemWidth(-1);
      if (ImGui::BeginCombo("##CurrentAnim",
                            anim->currentAnimationName.c_str()))
      {
        for (auto const &[name, data] : anim->animations)
        {
          bool isSelected = (anim->currentAnimationName == name);
          if (ImGui::Selectable(name.c_str(), isSelected))
          {
            anim->play(name);
          }
          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
      ImGui::PopItemWidth();

      // Scrubbing and Config
      if (anim->animations.count(anim->currentAnimationName))
      {
        auto &currentData = anim->animations[anim->currentAnimationName];

        ImGui::Columns(2, "AnimColumns", false);
        ImGui::SetColumnWidth(0, 80.0f);

        ImGui::Text("Cur Frame");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::SliderInt("##Frame", &anim->currentFrame, 0,
                         currentData.frameCount - 1);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Text("Max Frames");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if (ImGui::DragInt("##MaxFrames", &currentData.frameCount, 1.0f, 1,
                           128))
        {
          if (anim->currentFrame >= currentData.frameCount)
          {
            anim->currentFrame = currentData.frameCount - 1;
          }
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Text("Speed");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat("##Speed", &currentData.speed, 0.01f, 0.01f, 10.0f);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Text("Row");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragInt("##Row", &currentData.row, 1.0f, 0, 100);
        ImGui::PopItemWidth();

        ImGui::Columns(1);
      }

      ImGui::Separator();

      ImGui::Text("Add New State:");
      static char newAnimName[64] = "";
      ImGui::PushItemWidth(-1);
      ImGui::InputTextWithHint("##NewAnimName", "State Name (e.g. Run)",
                               newAnimName, 64);
      if (ImGui::Button("Add Animation State", ImVec2(-1, 0)))
      {
        if (strlen(newAnimName) > 0)
        {
          anim->addAnimation(newAnimName, 0, 1, 0.1f);
          newAnimName[0] = '\0';
        }
      }
      ImGui::PopItemWidth();

      ImGui::Separator();
      if (ImGui::Button("Remove Animation Component", ImVec2(-1, 0)))
      {
        m_SelectedEntity->removeComponent<Engine::AnimationComponent>();
      }
      ImGui::PopID();
    }
  }

  if (m_SelectedEntity->hasComponent<Engine::ScriptComponent>())
  {
    auto script = m_SelectedEntity->getComponent<Engine::ScriptComponent>();
    if (ImGui::CollapsingHeader("Script"))
    {
      ImGui::PushID("Script");
      if (ImGui::BeginPopupContextItem("ScriptContext"))
      {
        if (ImGui::MenuItem("Remove Script Component"))
        {
          m_SelectedEntity->removeComponent<Engine::ScriptComponent>();
        }
        ImGui::EndPopup();
      }

      ImGui::Text("Script:");
      std::string filename = std::filesystem::path(script->scriptPath).filename().string();
      char pathBuf[512];
      strncpy(pathBuf, filename.c_str(), sizeof(pathBuf));
      pathBuf[sizeof(pathBuf) - 1] = '\0';

      ImGui::PushItemWidth(-1);
      if (ImGui::InputText("##ScriptName", pathBuf, sizeof(pathBuf)))
      {
        std::filesystem::path fullPath(script->scriptPath);
        fullPath = fullPath.parent_path() / pathBuf;
        script->scriptPath = fullPath.string();
        script->isInitialized = false;
      }
      ImGui::PopItemWidth();

      if (ImGui::BeginDragDropTarget())
      {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ASSET_ID"))
        {
          const char *assetPath = (const char *)payload->Data;
          std::string droppedPath(assetPath);

          // Only accept Lua scripts
          if (std::filesystem::path(droppedPath).extension() == ".lua")
          {
            script->isInitialized = false;
            script->scriptPath = (m_currentProject->getAssetPath() / droppedPath).string();
            currentScene->getSystem<Engine::ScriptSystem>()
                ->reloadScript(script->scriptPath);
          }
        }
        ImGui::EndDragDropTarget();
      }

      ImGui::Text("Status: ");
      ImGui::SameLine();
      if (script->isInitialized)
      {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Initialized");
      }
      else
      {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Reloading..");
      }

      if (ImGui::Button("Reload & Initialize", ImVec2(-1, 0)))
      {
        script->isInitialized = false;
        currentScene->getSystem<Engine::ScriptSystem>()->reloadScript(
            script->scriptPath);
      }

      ImGui::Separator();
      if (ImGui::Button("Remove Script Component", ImVec2(-1, 0)))
      {
        m_SelectedEntity->removeComponent<Engine::ScriptComponent>();
      }
      ImGui::PopID();
    }
  }

  if (m_SelectedEntity->hasComponent<Engine::SoundComponent>())
  {
    auto *audio = m_SelectedEntity->getComponent<Engine::SoundComponent>();
    if (ImGui::CollapsingHeader("Audio Source"))
    {
      ImGui::PushID("Sound");
      if (ImGui::BeginPopupContextItem("AudioContext"))
      {
        if (ImGui::MenuItem("Remove Audio Component"))
        {
          m_SelectedEntity->removeComponent<Engine::SoundComponent>();
        }
        ImGui::EndPopup();
      }

      float availWidth = ImGui::GetContentRegionAvail().x;
      if (ImGui::Button("Preview sound", ImVec2(availWidth * 0.5f - 4.0f, 0)))
      {
        if (!audio->srcPath.empty())
        {
          Mix_Chunk *tempChunk = Mix_LoadWAV(audio->srcPath.c_str());
          Mix_VolumeChunk(tempChunk, static_cast<int>(audio->volume * MIX_MAX_VOLUME));

          if (tempChunk)
          {
            int channel = Mix_PlayChannel(-1, tempChunk, 0);

            if (channel != -1)
            {
              Mix_ExpireChannel(channel, (tempChunk->alen / 176.4) + 100);
            }
          }
          else
          {
            std::cerr << "SDL_mixer Error: " << Mix_GetError() << std::endl;
          }
        }
      }

      ImGui::SameLine();

      if (ImGui::Button("Stop preview", ImVec2(availWidth * 0.5f - 4.0f, 0)))
      {
        Mix_HaltChannel(-1);
      }

      ImGui::Separator();
      ImGui::Text("Audio Clip:");

      bool isDraggingAsset = false;
      if (const ImGuiPayload *payload = ImGui::GetDragDropPayload())
      {
        if (payload->IsDataType("ASSET_ID"))
          isDraggingAsset = true;
      }

      if (isDraggingAsset)
      {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.4f, 0.2f, 1.0f));
      }

      char pathBuf[512];
      memset(pathBuf, 0, sizeof(pathBuf));
      strncpy(pathBuf, audio->srcPath.c_str(), sizeof(pathBuf) - 1);

      ImGui::PushItemWidth(-1);

      if (ImGui::InputTextWithHint("##AudioPath", "Drag sound here...", pathBuf, sizeof(pathBuf)))
      {
        audio->srcPath = pathBuf;
      }

      if (ImGui::BeginDragDropTarget())
      {
        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ASSET_ID"))
        {
          const char *droppedPath = (const char *)payload->Data;

          std::string pathStr = droppedPath;
          std::string ext = pathStr.substr(pathStr.find_last_of(".") + 1);
          std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

          if (ext == "wav" || ext == "mp3" || ext == "ogg")
          {
            std::filesystem::path assetPath = m_currentProject->getAssetPath();
            audio->srcPath = (assetPath / pathStr).string();
          }
        }
        ImGui::EndDragDropTarget();
      }

      ImGui::PopItemWidth();
      if (isDraggingAsset)
        ImGui::PopStyleColor();

      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Current Path: %s", audio->srcPath.c_str());
      }

      ImGui::SliderFloat("Volume", &audio->volume, 0.0f, 1.0f);
      ImGui::Checkbox("Looping", &audio->loop);

      if (audio->channel != -1)
      {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Playing on Channel: %d", audio->channel);
      }
      else
      {
        ImGui::TextDisabled("Status: Idle");
      }

      ImGui::Separator();
      if (ImGui::Button("Remove Audio Source", ImVec2(-1, 0)))
      {
        m_SelectedEntity->removeComponent<Engine::SoundComponent>();
      }
      ImGui::PopID();
    }
  }
  if (m_SelectedEntity->hasComponent<Engine::TextComponent>())
  {
    auto textComp = m_SelectedEntity->getComponent<Engine::TextComponent>();

    if (ImGui::CollapsingHeader("Text Component"))
    {
      ImGui::PushID("Text");
      // --- Text Content ---
      char textBuffer[512];
      strncpy(textBuffer, textComp->text.c_str(), sizeof(textBuffer));
      if (ImGui::InputTextMultiline("Text", textBuffer, sizeof(textBuffer)))
      {
        textComp->text = textBuffer;
        textComp->dirty = true;
      }

      // --- Font Path / Font Combo ---
      std::vector<std::string> fontList = m_AssetManager->getAllFontAssetIds();
      int currentFontIndex = -1;
      for (int i = 0; i < fontList.size(); ++i)
        if (fontList[i] == textComp->fontPath)
          currentFontIndex = i;

      const char *comboLabel = (currentFontIndex >= 0) ? fontList[currentFontIndex].c_str() : "None";
      if (ImGui::BeginCombo("Font", comboLabel))
      {
        if (ImGui::Selectable("None", currentFontIndex == -1))
        {
          textComp->fontPath.clear();
          textComp->font = nullptr;
          textComp->dirty = true;
          currentFontIndex = -1;
        }

        for (int i = 0; i < fontList.size(); ++i)
        {
          bool isSelected = (currentFontIndex == i);
          if (ImGui::Selectable(fontList[i].c_str(), isSelected))
          {
            textComp->fontPath = fontList[i];
            textComp->font = m_AssetManager->getFont(fontList[i]);
            textComp->dirty = true;
            currentFontIndex = i;
          }
          if (isSelected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      int fontSize = textComp->fontSize;
      if (ImGui::DragInt("Font Size", &fontSize, 1, 1, 512))
      {
        if (fontSize != textComp->fontSize)
        {
          textComp->fontSize = fontSize;

          if (!textComp->fontPath.empty())
          {
            m_AssetManager->removeFont(textComp->fontPath);
            auto newFont = m_AssetManager->loadFontIfMissing(textComp->fontPath, textComp->fontPath, textComp->fontSize, m_currentProject);
            if (newFont)
            {
              textComp->font = newFont;
              textComp->dirty = true;
            }
            else
            {
              Engine::Log::error(("TextInspector: Failed to load font '" + textComp->fontPath +
                                  "' with size " + std::to_string(textComp->fontSize)));
            }
          }
        }
      }

      float color[4] = {
          textComp->color.r / 255.0f,
          textComp->color.g / 255.0f,
          textComp->color.b / 255.0f,
          textComp->color.a / 255.0f};
      if (ImGui::ColorEdit4("Color", color))
      {
        Uint8 r = static_cast<Uint8>(color[0] * 255);
        Uint8 g = static_cast<Uint8>(color[1] * 255);
        Uint8 b = static_cast<Uint8>(color[2] * 255);
        Uint8 a = static_cast<Uint8>(color[3] * 255);

        if (r != textComp->color.r || g != textComp->color.g || b != textComp->color.b || a != textComp->color.a)
        {
          textComp->color = {r, g, b, a};
          textComp->dirty = true;
        }
      }
      // --- Fixed Position ---
      if (ImGui::Checkbox("Fixed On Screen", &textComp->isFixed))
        textComp->dirty = true;

      // --- Z Index ---
      if (ImGui::DragInt("Z Index", &textComp->zIndex, 1))
        textComp->dirty = true;

      // --- Remove Component ---
      if (ImGui::Button("Remove Text Component", ImVec2(-1, 0)))
      {
        m_SelectedEntity->removeComponent<Engine::TextComponent>();
      }

      ImGui::Separator();
      ImGui::PopID();
    }
  }
  if (m_SelectedEntity->hasComponent<Engine::CameraComponent>())
  {
    bool open = ImGui::CollapsingHeader("Camera");

    ImGui::PushID("Camera");
    if (ImGui::BeginPopupContextItem("CameraContext"))
    {
      if (ImGui::MenuItem("Remove Camera Component"))
      {
        m_SelectedEntity->removeComponent<Engine::CameraComponent>();
      }
      ImGui::EndPopup();
    }

    if (open)
    {
      auto *cameraComponent = m_SelectedEntity->getComponent<Engine::CameraComponent>();

      ImGui::Checkbox("Active", &cameraComponent->isPrimary);

      ImGui::Separator();

      if (cameraComponent->isPrimary)
      {
        ImGui::TextDisabled("Scene Camera Settings");

        auto *sceneCamera = currentScene->getSceneCamera();
        if (sceneCamera)
        {
          float orthoSize = sceneCamera->getOrthographicSize();
          if (ImGui::DragFloat("Orthographic Size", &orthoSize, 0.1f, 0.1f, 1000.0f))
          {
            sceneCamera->setOrthographicSize(orthoSize);
          }

          float nearClip = sceneCamera->getNearClip();
          if (ImGui::DragFloat("Near Clip", &nearClip, 0.1f))
          {
            sceneCamera->setNearClip(nearClip);
          }

          float farClip = sceneCamera->getFarClip();
          if (ImGui::DragFloat("Far Clip", &farClip, 0.1f))
          {
            sceneCamera->setFarClip(farClip);
          }
        }
      }
      else
      {
        ImGui::TextDisabled("This is not the primary camera. Its transform does not affect the main view.");
      }

      ImGui::Separator();
      if (ImGui::Button("Remove Camera Component", ImVec2(-1, 0)))
      {
        m_SelectedEntity->removeComponent<Engine::CameraComponent>();
      }
    }
    ImGui::PopID();
  }

  if (m_SelectedEntity->hasComponent<Engine::InputControllerComponent>())
  {
    auto inputComp = m_SelectedEntity->getComponent<Engine::InputControllerComponent>();

    if (ImGui::CollapsingHeader("Input Controller"))
    {
      ImGui::PushID("InputController");
      if (ImGui::BeginPopupContextItem("InputControllerContext"))
      {
        if (ImGui::MenuItem("Remove Input Controller Component"))
        {
          m_SelectedEntity->removeComponent<Engine::InputControllerComponent>();
        }
        ImGui::EndPopup();
      }

      // --- Parameters ---
      ImGui::Text("Parameters");
      ImGui::Separator();

      // Optional: sort parameters alphabetically
      std::vector<std::pair<std::string, float *>> sortedParams;
      for (auto &[name, value] : inputComp->parameters)
      {
        sortedParams.emplace_back(name, &value);
      }
      std::sort(sortedParams.begin(), sortedParams.end(),
                [](auto &a, auto &b)
                { return a.first < b.first; });

      // Table layout
      if (ImGui::BeginTable("ParamsTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV))
      {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Value");
        ImGui::TableSetupColumn("##Actions", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        for (auto &[paramName, valuePtr] : sortedParams)
        {
          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          ImGui::Text("%s", paramName.c_str());

          ImGui::TableSetColumnIndex(1);
          ImGui::PushID(paramName.c_str());
          ImGui::DragFloat("##Value", valuePtr, 0.1f);
          ImGui::PopID();

          ImGui::TableSetColumnIndex(2);
          ImGui::PushID((paramName + "_del").c_str());
          if (ImGui::Button("X"))
          {
            inputComp->parameters.erase(paramName);
            ImGui::PopID();
            break;
          }
          ImGui::PopID();
        }

        ImGui::EndTable();
      }

      ImGui::Spacing();
      ImGui::Separator();
      ImGui::Text("Add Parameter");

      // Add new parameter section
      static char newParamName[64] = "";
      static float newParamValue = 0.0f;

      ImGui::PushItemWidth(150);
      ImGui::InputTextWithHint("##NewParamName", "Parameter Name", newParamName, sizeof(newParamName));
      ImGui::SameLine();
      ImGui::DragFloat("##NewParamValue", &newParamValue, 0.1f);
      ImGui::SameLine();

      bool canAdd = strlen(newParamName) > 0;

      ImGui::BeginDisabled(!canAdd);
      if (ImGui::Button("Add") || (canAdd && ImGui::IsKeyPressed(ImGuiKey_Enter)))
      {
        inputComp->addParameter(newParamName, newParamValue);
        newParamName[0] = '\0';
        newParamValue = 0.0f;
        ImGui::SetKeyboardFocusHere(-1);
      }
      ImGui::EndDisabled();

      ImGui::PopItemWidth();
      ImGui::Separator();

      // --- Action Mappings ---
      ImGui::Text("Key Mappings:");
      for (size_t i = 0; i < inputComp->mappings.size(); ++i)
      {
        auto &mapping = inputComp->mappings[i];

        char buf[64];
        strncpy(buf, mapping.actionName.c_str(), sizeof(buf));
        if (ImGui::InputText(("Action##" + std::to_string(i)).c_str(), buf, sizeof(buf)))
        {
          mapping.actionName = buf;
        }

        const char *keyName = SDL_GetScancodeName(mapping.scancode);
        if (strlen(keyName) == 0)
          keyName = "Unknown";

        if (ImGui::BeginCombo(("Key##" + std::to_string(i)).c_str(), keyName))
        {
          for (int k = 0; k < SDL_NUM_SCANCODES; k++)
          {
            const char *name = SDL_GetScancodeName((SDL_Scancode)k);
            if (name && strlen(name) > 0)
            {
              if (ImGui::Selectable(name, mapping.scancode == (SDL_Scancode)k))
                mapping.scancode = (SDL_Scancode)k;
            }
          }
          ImGui::EndCombo();
        }

        ImGui::SameLine();
        if (ImGui::Button(("X##" + std::to_string(i)).c_str()))
        {
          inputComp->mappings.erase(inputComp->mappings.begin() + i);
          --i; // Adjust index after erase
        }
      }

      // Add new mapping
      static char newAction[64] = "";
      static int newKey = SDL_SCANCODE_UNKNOWN;
      ImGui::InputText("New Action", newAction, sizeof(newAction));

      const char *newKeyName = SDL_GetScancodeName((SDL_Scancode)newKey);
      if (strlen(newKeyName) == 0)
        newKeyName = "Select Key";

      if (ImGui::BeginCombo("New Key", newKeyName))
      {
        for (int k = 0; k < SDL_NUM_SCANCODES; k++)
        {
          const char *name = SDL_GetScancodeName((SDL_Scancode)k);
          if (name && strlen(name) > 0)
          {
            if (ImGui::Selectable(name, newKey == k))
              newKey = k;
          }
        }
        ImGui::EndCombo();
      }

      if (ImGui::Button("Add Mapping"))
      {
        if (strlen(newAction) > 0 && newKey != SDL_SCANCODE_UNKNOWN)
        {
          inputComp->addMapping(newAction, static_cast<SDL_Scancode>(newKey));
          newAction[0] = '\0';
          newKey = SDL_SCANCODE_UNKNOWN;
        }
      }

      ImGui::Separator();

      ImGui::Checkbox("Override Default Settings", &inputComp->overrideDefaultSettings);

      if (ImGui::Button("Remove Input Controller Component", ImVec2(-1, 0)))
      {
        m_SelectedEntity->removeComponent<Engine::InputControllerComponent>();
      }
      ImGui::PopID();
    }
  }

  if (m_SelectedEntity->hasComponent<Engine::RigidBodyComponent>())
  {
    bool open =
        ImGui::CollapsingHeader("Rigid Body");

    ImGui::PushID("RigidBody");
    if (ImGui::BeginPopupContextItem("RigidBodyContext"))
    {
      if (ImGui::MenuItem("Remove RigidBody Component"))
      {
        m_SelectedEntity->removeComponent<Engine::RigidBodyComponent>();
      }
      ImGui::EndPopup();
    }

    if (open)
    {
      auto rb = m_SelectedEntity->getComponent<Engine::RigidBodyComponent>();
      if (rb)
      {
        // Body Type Combo
        const char *bodyTypes[] = {"Static", "Dynamic", "Kinematic"};
        int currentType = (int)rb->bodyType;
        if (ImGui::Combo("Body Type", &currentType, bodyTypes,
                         IM_ARRAYSIZE(bodyTypes)))
        {
          rb->bodyType = (Engine::BodyType)currentType;
        }

        ImGui::DragFloat("Mass", &rb->mass, 0.1f, 0.001f, 1000.0f);
        ImGui::DragFloat("Gravity Scale", &rb->gravityScale, 0.1f, -10.0f,
                         10.0f);
        ImGui::DragFloat("Linear Drag", &rb->linearDrag, 0.01f, 0.0f, 1.0f);

        ImGui::Separator();
        ImGui::Text("Physics State");
        ImGui::DragFloat2("Velocity", &rb->velocity.x, 0.1f);
        ImGui::DragFloat2("Acceleration", &rb->acceleration.x, 0.1f);

        if (ImGui::Button("Reset Velocity"))
        {
          rb->velocity = {0, 0};
          rb->acceleration = {0, 0};
        }
      }
    }
    ImGui::PopID();
  }

  if (m_SelectedEntity->hasComponent<Engine::VelocityComponent>())
  {
    bool open = ImGui::CollapsingHeader("Velocity");

    ImGui::PushID("Velocity");
    if (ImGui::BeginPopupContextItem("VelocityContext"))
    {
      if (ImGui::MenuItem("Remove Velocity Component"))
      {
        m_SelectedEntity->removeComponent<Engine::VelocityComponent>();
      }
      ImGui::EndPopup();
    }

    if (open)
    {
      auto *vc = m_SelectedEntity->getComponent<Engine::VelocityComponent>();
      if (vc)
      {
        ImGui::DragFloat2("Velocity", &vc->velocity.x, 0.1f);

        if (ImGui::Button("Reset Velocity##Velocity", ImVec2(-1, 0)))
          vc->velocity = {0.0f, 0.0f};

        ImGui::Separator();
        if (ImGui::Button("Remove Velocity Component##Velocity", ImVec2(-1, 0)))
        {
          m_SelectedEntity->removeComponent<Engine::VelocityComponent>();
        }
      }
    }
    ImGui::PopID();
  }

  if (m_SelectedEntity->hasComponent<Engine::BoxColliderComponent>())
  {
    bool open =
        ImGui::CollapsingHeader("Box Collider");

    ImGui::PushID("BoxCollider");
    if (ImGui::BeginPopupContextItem("BoxColliderContext"))
    {
      if (ImGui::MenuItem("Remove BoxCollider Component"))
      {
        m_SelectedEntity->removeComponent<Engine::BoxColliderComponent>();
      }
      ImGui::EndPopup();
    }

    if (open)
    {
      auto bc = m_SelectedEntity->getComponent<Engine::BoxColliderComponent>();
      if (bc)
      {
        ImGui::DragFloat2("Size", &bc->size.x, 1.0f, 0.1f, 2048.0f);
        ImGui::DragFloat2("Offset", &bc->offset.x, 1.0f);
        ImGui::DragFloat("Box Rotation", &bc->rotation, 1.0f, 360.0f);
        if (ImGui::Button("Edit in viewport"))
        {
          g_EditColliderMode = !g_EditColliderMode;
        }

        ImGui::Separator();
        ImGui::Checkbox("Is Trigger", &bc->isTrigger);
        ImGui::Checkbox("Is Static", &bc->isStatic);

        ImGui::Separator();
        ImGui::Text("Collision Filtering");

        // Helper for bitmask flags
        auto LayerFlag = [&](const char *label, Engine::CollisionLayer flag,
                             uint32_t &mask)
        {
          bool val = (mask & flag);
          if (ImGui::Checkbox(label, &val))
          {
            if (val)
              mask |= flag;
            else
              mask &= ~flag;
          }
        };

        if (ImGui::TreeNode("Layer (Who am I?)"))
        {
          uint32_t layer = bc->layer;
          LayerFlag("Default", Engine::CollisionLayer::Default, layer);
          LayerFlag("Player", Engine::CollisionLayer::Player, layer);
          LayerFlag("Enemy", Engine::CollisionLayer::Enemy, layer);
          LayerFlag("Obstacle", Engine::CollisionLayer::Obstacle, layer);
          LayerFlag("Projectile", Engine::CollisionLayer::Projectile, layer);
          LayerFlag("Trigger", Engine::CollisionLayer::Trigger, layer);
          bc->layer = layer;
          ImGui::TreePop();
        }

        if (ImGui::TreeNode("Mask (Who hits me?)"))
        {
          uint32_t mask = bc->mask;
          LayerFlag("Default", Engine::CollisionLayer::Default, mask);
          LayerFlag("Player", Engine::CollisionLayer::Player, mask);
          LayerFlag("Enemy", Engine::CollisionLayer::Enemy, mask);
          LayerFlag("Obstacle", Engine::CollisionLayer::Obstacle, mask);
          LayerFlag("Projectile", Engine::CollisionLayer::Projectile, mask);
          LayerFlag("Trigger", Engine::CollisionLayer::Trigger, mask);
          bc->mask = mask;
          ImGui::TreePop();
        }
      }
    }
    ImGui::PopID();
  }
  if (m_SelectedEntity->hasComponent<Engine::CircleColliderComponent>())
  {
    bool open = ImGui::CollapsingHeader("Circle Collider");

    ImGui::PushID("CircleCollider");
    if (ImGui::BeginPopupContextItem("CircleColliderContext"))
    {
      if (ImGui::MenuItem("Remove CircleCollider Component"))
      {
        m_SelectedEntity->removeComponent<Engine::CircleColliderComponent>();
      }
      ImGui::EndPopup();
    }

    if (open)
    {
      auto cc =
          m_SelectedEntity->getComponent<Engine::CircleColliderComponent>();
      if (cc)
      {
        ImGui::DragFloat("Radius", &cc->radius, 1.0f, 0.1f, 1024.0f);
        ImGui::DragFloat2("Offset##circle", glm::value_ptr(cc->offset), 1.0f);

        if (ImGui::Button("Edit in viewport##Circle"))
        {
          g_EditColliderMode = !g_EditColliderMode;
        }

        ImGui::Separator();
        ImGui::Checkbox("Is Trigger##Circle", &cc->isTrigger);
        ImGui::Checkbox("Is Static##Circle", &cc->isStatic);

        ImGui::Separator();
        ImGui::Text("Collision Filtering");

        auto LayerFlag = [&](const char *label, Engine::CollisionLayer flag,
                             uint32_t &mask)
        {
          bool val = (mask & flag);
          if (ImGui::Checkbox(label, &val))
          {
            if (val)
              mask |= flag;
            else
              mask &= ~flag;
          }
        };

        if (ImGui::TreeNode("Layer (Who am I?)##Circle"))
        {
          uint32_t layer = cc->layer;
          LayerFlag("Default", Engine::CollisionLayer::Default, layer);
          LayerFlag("Player", Engine::CollisionLayer::Player, layer);
          LayerFlag("Enemy", Engine::CollisionLayer::Enemy, layer);
          LayerFlag("Obstacle", Engine::CollisionLayer::Obstacle, layer);
          LayerFlag("Projectile", Engine::CollisionLayer::Projectile, layer);
          LayerFlag("Trigger", Engine::CollisionLayer::Trigger, layer);
          cc->layer = layer;
          ImGui::TreePop();
        }

        if (ImGui::TreeNode("Mask (Who hits me?)##Circle"))
        {
          uint32_t mask = cc->mask;
          LayerFlag("Default", Engine::CollisionLayer::Default, mask);
          LayerFlag("Player", Engine::CollisionLayer::Player, mask);
          LayerFlag("Enemy", Engine::CollisionLayer::Enemy, mask);
          LayerFlag("Obstacle", Engine::CollisionLayer::Obstacle, mask);
          LayerFlag("Projectile", Engine::CollisionLayer::Projectile, mask);
          LayerFlag("Trigger", Engine::CollisionLayer::Trigger, mask);
          cc->mask = mask;
          ImGui::TreePop();
        }
      }
    }
    ImGui::PopID();
  }

  if (m_SelectedEntity->hasComponent<Engine::PolygonColliderComponent>())
  {
    bool open = ImGui::CollapsingHeader("Polygon Collider");

    ImGui::PushID("PolygonCollider");
    if (ImGui::BeginPopupContextItem("PolygonColliderContext"))
    {
      if (ImGui::MenuItem("Remove PolygonCollider Component"))
      {
        m_SelectedEntity->removeComponent<Engine::PolygonColliderComponent>();
      }
      ImGui::EndPopup();
    }

    if (open)
    {
      auto pc =
          m_SelectedEntity->getComponent<Engine::PolygonColliderComponent>();
      if (pc)
      {
        ImGui::DragFloat2("Offset##polygon", glm::value_ptr(pc->offset), 1.0f);
        ImGui::DragFloat("Local Rotation", &pc->rotation, 0.5f);

        if (ImGui::Button("Edit in viewport##Poly"))
        {
          g_EditColliderMode = !g_EditColliderMode;
        }

        ImGui::Separator();
        ImGui::Checkbox("Is Trigger##Poly", &pc->isTrigger);
        ImGui::Checkbox("Is Static##Poly", &pc->isStatic);

        ImGui::Separator();
        ImGui::Text("Vertices (%d):", (int)pc->vertices.size());
        ImGui::Indent();
        for (size_t i = 0; i < pc->vertices.size(); ++i)
        {
          std::string label = "Point " + std::to_string(i);
          ImGui::PushID((int)i);
          ImGui::DragFloat2(label.c_str(), glm::value_ptr(pc->vertices[i]), 1.0f);
          ImGui::SameLine();
          if (ImGui::Button("X"))
          {
            pc->vertices.erase(pc->vertices.begin() + i);
            ImGui::PopID();
            break;
          }
          ImGui::PopID();
        }
        if (ImGui::Button("+ Add Vertex"))
        {
          pc->vertices.push_back({0, 0});
        }
        ImGui::Unindent();

        ImGui::Separator();
        ImGui::Text("Collision Filtering");

        auto LayerFlag = [&](const char *label, Engine::CollisionLayer flag,
                             uint32_t &mask)
        {
          bool val = (mask & flag);
          if (ImGui::Checkbox(label, &val))
          {
            if (val)
              mask |= flag;
            else
              mask &= ~flag;
          }
        };

        if (ImGui::TreeNode("Layer (Who am I?)##Circle"))
        {
          uint32_t layer = pc->layer;
          LayerFlag("Default", Engine::CollisionLayer::Default, layer);
          LayerFlag("Player", Engine::CollisionLayer::Player, layer);
          LayerFlag("Enemy", Engine::CollisionLayer::Enemy, layer);
          LayerFlag("Obstacle", Engine::CollisionLayer::Obstacle, layer);
          LayerFlag("Projectile", Engine::CollisionLayer::Projectile, layer);
          LayerFlag("Trigger", Engine::CollisionLayer::Trigger, layer);
          pc->layer = layer;
          ImGui::TreePop();
        }

        if (ImGui::TreeNode("Mask (Who hits me?)##Circle"))
        {
          uint32_t mask = pc->mask;
          LayerFlag("Default", Engine::CollisionLayer::Default, mask);
          LayerFlag("Player", Engine::CollisionLayer::Player, mask);
          LayerFlag("Enemy", Engine::CollisionLayer::Enemy, mask);
          LayerFlag("Obstacle", Engine::CollisionLayer::Obstacle, mask);
          LayerFlag("Projectile", Engine::CollisionLayer::Projectile, mask);
          LayerFlag("Trigger", Engine::CollisionLayer::Trigger, mask);
          pc->mask = mask;
          ImGui::TreePop();
        }
      }
    }
    ImGui::PopID();
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
  if (ImGui::Button("Add Component",
                    ImVec2(ImGui::GetContentRegionAvail().x, 0)))
  {
    ImGui::OpenPopup("AddComponentPopup");
  }

  if (ImGui::BeginPopup("AddComponentPopup"))
  {
    if (ImGui::MenuItem("RigidBody"))
    {
      m_SelectedEntity->addComponent<Engine::RigidBodyComponent>();
    }
    if (ImGui::MenuItem("Script"))
    {
      m_SelectedEntity->addComponent<Engine::ScriptComponent>();
    }
    if (ImGui::MenuItem("Sprite"))
    {
      m_SelectedEntity->addComponent<Engine::SpriteComponent>();
    }
    if (ImGui::MenuItem("Animation"))
    {
      m_SelectedEntity->addComponent<Engine::AnimationComponent>();
    }
    if (ImGui::MenuItem("Camera"))
    {
      m_SelectedEntity->addComponent<Engine::CameraComponent>();
    }
    if (ImGui::MenuItem("Input Controller"))
    {
      m_SelectedEntity->addComponent<Engine::InputControllerComponent>();
    }
    if (ImGui::MenuItem("Velocity"))
    {
      m_SelectedEntity->addComponent<Engine::VelocityComponent>();
    }
    if (ImGui::MenuItem("Circle Collider"))
    {
      m_SelectedEntity->addComponent<Engine::CircleColliderComponent>();
    }
    if (ImGui::MenuItem("Polygon Collider"))
    {
      m_SelectedEntity->addComponent<Engine::PolygonColliderComponent>();
    }
    if (ImGui::MenuItem("Box Collider"))
    {
      m_SelectedEntity->addComponent<Engine::BoxColliderComponent>();
    }
    if (ImGui::MenuItem("Sound"))
    {
      m_SelectedEntity->addComponent<Engine::SoundComponent>();
    }
    if (ImGui::MenuItem("Text"))
    {
      auto comp = m_SelectedEntity->addComponent<Engine::TextComponent>();
      comp->dirty = true;
      comp->fontPath = "assets/fonts/Default.ttf";
    }
    ImGui::EndPopup();
  }

  ImGui::End();
}