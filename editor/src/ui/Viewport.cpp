#include "../EditorApp.h"
#include "Gizmos.h"
#include "ImGuizmo.h"
#include "SDL_render.h"
#include "core/Camera.h"
#include "ecs/Entity.h"
#include "ecs/components/BoxColliderComponent.h"
#include "ecs/components/CircleColliderComponent.h"
#include "ecs/components/PolygonColliderComponent.h"
#include "ecs/components/SpriteComponent.h"
#include "ecs/components/TransformComponent.h"
#include "glm/ext/quaternion_common.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "scene/Scene.h"
#include <imgui.h>
#include <vector>

glm::mat4 GetWorldMatrix(Engine::Entity *e) {
  if (!e)
    return glm::mat4(1.0f);
  auto tr = e->getComponent<Engine::TransformComponent>();
  if (!tr)
    return glm::mat4(1.0f);

  glm::mat4 local = glm::translate(
      glm::mat4(1.0f), glm::vec3(tr->position.x, tr->position.y, 0.0f));
  local = glm::rotate(local, glm::radians(tr->rotation), glm::vec3(0, 0, 1));
  local = glm::scale(local, glm::vec3(tr->scale.x, tr->scale.y, 1.0f));

  if (e->getParent()) {
    return GetWorldMatrix(e->getParent()) * local;
  }
  return local;
}

void DrawSceneBackground(SDL_Renderer *renderer, Engine::Scene *scene,
                         Engine::AssetManager *assetManager, float w, float h) {
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

void EditorApp::renderViewport() {
  static Engine::Camera s_EditorCamera;
  static int s_DraggingVertexIndex = -1;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::Begin("Viewport");

  ImVec2 availableSize = ImGui::GetContentRegionAvail();
  float aspectRatio = 16.0f / 9.0f;
  ImVec2 newViewportSize = availableSize;

  if (newViewportSize.x / newViewportSize.y > aspectRatio) {
    newViewportSize.x = newViewportSize.y * aspectRatio;
  } else {
    newViewportSize.y = newViewportSize.x / aspectRatio;
  }
  ImVec2 mousePos = ImGui::GetIO().MousePos;

  ImVec2 cursor = ImGui::GetCursorPos();
  if (availableSize.x > newViewportSize.x)
    cursor.x += (availableSize.x - newViewportSize.x) * 0.5f;
  if (availableSize.y > newViewportSize.y)
    cursor.y += (availableSize.y - newViewportSize.y) * 0.5f;
  ImGui::SetCursorPos(cursor);

  viewportPos = ImGui::GetCursorScreenPos();
  ImVec2 currentViewportSize = newViewportSize;

  if (currentViewportSize.x <= 1 || currentViewportSize.y <= 1) {
    ImGui::End();
    ImGui::PopStyleVar();
    return;
  }

  if (currentViewportSize.x != viewportSize.x ||
      currentViewportSize.y != viewportSize.y) {
    viewportSize = currentViewportSize;
    if (m_GameRenderTarget)
      SDL_DestroyTexture(m_GameRenderTarget);
    m_GameRenderTarget = SDL_CreateTexture(
        m_Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        (int)viewportSize.x, (int)viewportSize.y);
  }

  float renderW = (float)(int)viewportSize.x;
  float renderH = (float)(int)viewportSize.y;
  s_EditorCamera.setAspectRatio(renderW / renderH);

  // Navigation logic handled here...
  if (m_SceneState == SceneState::EDIT && ImGui::IsWindowHovered()) {
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
      ImVec2 delta = ImGui::GetIO().MouseDelta;
      float unitsPerPixelX = (s_EditorCamera.getOrthographicSize() *
                              s_EditorCamera.getAspectRatio()) /
                             viewportSize.x;
      float unitsPerPixelY =
          s_EditorCamera.getOrthographicSize() / viewportSize.y;
      glm::vec2 pos = s_EditorCamera.getPosition();
      pos.x -= delta.x * unitsPerPixelX;
      pos.y += delta.y * unitsPerPixelY;
      s_EditorCamera.setPosition(pos);
    }
    if (ImGui::GetIO().MouseWheel != 0) {
      float newSize = s_EditorCamera.getOrthographicSize() -
                      (ImGui::GetIO().MouseWheel *
                       s_EditorCamera.getOrthographicSize() * 0.1f);
      s_EditorCamera.setOrthographicSize(std::max(newSize, 1.0f));
    }
  }

  if (m_GameRenderTarget) {
    SDL_SetRenderTarget(m_Renderer, m_GameRenderTarget);
    
    if (currentScene) {
      DrawSceneBackground(m_Renderer, currentScene.get(), m_AssetManager.get(),
                          renderW, renderH);
    } else {
      SDL_SetRenderDrawColor(m_Renderer, 20, 20, 20, 255);
      SDL_RenderClear(m_Renderer);
    }

    if (currentScene) {
      Engine::Camera *renderCamera = (m_SceneState == SceneState::EDIT)
                                         ? &s_EditorCamera
                                         : currentScene->getSceneCamera();
      if (renderCamera)
        currentScene->render(m_Renderer, *renderCamera, renderW, renderH);
    }

    if (m_SceneState == SceneState::EDIT && currentScene) {
            Engine::Camera* gameCamera = currentScene->getSceneCamera();
            if (gameCamera) {
                glm::mat4 viewProj = s_EditorCamera.getProjectionMatrix() * s_EditorCamera.getViewMatrix();
                
                float orthoSize = gameCamera->getOrthographicSize();
                float aspect = gameCamera->getAspectRatio();
                
                float halfHeight = orthoSize;
                float halfWidth = orthoSize * aspect;
                glm::vec2 camPos = gameCamera->getPosition();

                glm::vec4 corners[4] = {
                    { camPos.x - halfWidth, camPos.y - halfHeight, 0.0f, 1.0f },
                    { camPos.x + halfWidth, camPos.y - halfHeight, 0.0f, 1.0f },
                    { camPos.x + halfWidth, camPos.y + halfHeight, 0.0f, 1.0f },
                    { camPos.x - halfWidth, camPos.y + halfHeight, 0.0f, 1.0f }
                };

                SDL_FPoint screenPoints[5];
                for (int i = 0; i < 4; i++) {
                    glm::vec4 projected = viewProj * corners[i];
                    if (projected.w != 0.0f) {
                        glm::vec3 ndc = glm::vec3(projected.x / projected.w, projected.y / projected.w, projected.z / projected.w);
                        screenPoints[i] = {
                            (ndc.x + 1.0f) * 0.5f * renderW,
                            (1.0f - ndc.y) * 0.5f * renderH
                        };
                    }
                }
                screenPoints[4] = screenPoints[0]; 

                SDL_SetRenderDrawColor(m_Renderer, 255, 255, 255, 100);
                SDL_RenderDrawLinesF(m_Renderer, screenPoints, 5);

            }
        }


    if (m_SceneState == SceneState::EDIT && m_SelectedEntity) {
      glm::mat4 viewMatrix = s_EditorCamera.getViewMatrix();
      glm::mat4 projectionMatrix = s_EditorCamera.getProjectionMatrix();
      glm::mat4 modelMatrix = GetWorldMatrix(m_SelectedEntity);
      glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;

      // --- 1. Sprite/Selection Outline (Yellow) ---
      glm::vec2 entitySize = {32.0f, 32.0f};
      if (m_SelectedEntity->hasComponent<Engine::SpriteComponent>()) {
        auto sp = m_SelectedEntity->getComponent<Engine::SpriteComponent>();
        entitySize = {(float)sp->sourceRect.w, (float)sp->sourceRect.h};
      }

      SDL_FPoint screenOutline[5];
      glm::vec2 half = entitySize * 0.5f;
      glm::vec4 localCorners[4] = {{-half.x, -half.y, 0, 1},
                                   {half.x, -half.y, 0, 1},
                                   {half.x, half.y, 0, 1},
                                   {-half.x, half.y, 0, 1}};
      for (int i = 0; i < 4; i++) {
        glm::vec4 cp = mvp * localCorners[i];
        glm::vec3 ndc = glm::vec3(cp.x / cp.w, cp.y / cp.w, cp.z / cp.w);
        screenOutline[i] = {(ndc.x + 1.0f) * 0.5f * renderW,
                            (1.0f - ndc.y) * 0.5f * renderH};
      }
      screenOutline[4] = screenOutline[0];
      SDL_SetRenderDrawColor(m_Renderer, 255, 255, 0, 150);
      SDL_RenderDrawLinesF(m_Renderer, screenOutline, 5);

      SDL_SetRenderDrawColor(m_Renderer, 0, 255, 0, 255);

      if (m_SelectedEntity->hasComponent<Engine::BoxColliderComponent>()) {
        auto col =
            m_SelectedEntity->getComponent<Engine::BoxColliderComponent>();
        glm::vec2 half = col->size * 0.5f;

        glm::vec4 localCorners[4] = {{-half.x, -half.y, 0.01f, 1.0f},
                                     {half.x, -half.y, 0.01f, 1.0f},
                                     {half.x, half.y, 0.01f, 1.0f},
                                     {-half.x, half.y, 0.01f, 1.0f}};

        glm::mat4 colLocalTransform = glm::translate(
            glm::mat4(1.0f), glm::vec3(col->offset.x, col->offset.y, 0.0f));
        colLocalTransform = glm::rotate(
            colLocalTransform, glm::radians(col->rotation), glm::vec3(0, 0, 1));

        glm::mat4 fullColliderMVP =
            projectionMatrix * viewMatrix * modelMatrix * colLocalTransform;

        SDL_FPoint sCol[5];
        for (int i = 0; i < 4; i++) {
          glm::vec4 cp = fullColliderMVP * localCorners[i];

          if (cp.w != 0.0f) {
            glm::vec3 ndc = glm::vec3(cp.x / cp.w, cp.y / cp.w, cp.z / cp.w);
            sCol[i] = {(ndc.x + 1.0f) * 0.5f * renderW,
                       (1.0f - ndc.y) * 0.5f * renderH};
          }
        }
        sCol[4] = sCol[0];
        SDL_RenderDrawLinesF(m_Renderer, sCol, 5);
      }

      if (m_SelectedEntity->hasComponent<Engine::CircleColliderComponent>()) {
        auto col =
            m_SelectedEntity->getComponent<Engine::CircleColliderComponent>();
        const int segments = 32;
        SDL_FPoint sCol[segments + 1];
        for (int i = 0; i <= segments; i++) {
          float theta = (2.0f * 3.14159f * (float)i) / (float)segments;
          glm::vec4 localPos = {col->offset.x + cosf(theta) * col->radius,
                                col->offset.y + sinf(theta) * col->radius,
                                0.01f, 1.0f};
          glm::vec4 cp = mvp * localPos;
          glm::vec3 ndc = glm::vec3(cp.x / cp.w, cp.y / cp.w, cp.z / cp.w);
          sCol[i] = {(ndc.x + 1.0f) * 0.5f * renderW,
                     (1.0f - ndc.y) * 0.5f * renderH};
        }
        SDL_RenderDrawLinesF(m_Renderer, sCol, segments + 1);
      }

      if (m_SelectedEntity->hasComponent<Engine::PolygonColliderComponent>()) {
        auto col =
            m_SelectedEntity->getComponent<Engine::PolygonColliderComponent>();
        if (!col->vertices.empty()) {
          std::vector<SDL_FPoint> sCol;
          for (const auto &v : col->vertices) {
            glm::vec4 localPos = {v.x + col->offset.x, v.y + col->offset.y,
                                  0.01f, 1.0f};
            glm::vec4 cp = mvp * localPos;
            glm::vec3 ndc = glm::vec3(cp.x / cp.w, cp.y / cp.w, cp.z / cp.w);
            sCol.push_back({(ndc.x + 1.0f) * 0.5f * renderW,
                            (1.0f - ndc.y) * 0.5f * renderH});
          }
          sCol.push_back(sCol[0]); // Close polygon
          SDL_RenderDrawLinesF(m_Renderer, sCol.data(), (int)sCol.size());
        }
      }
    }

    SDL_SetRenderTarget(m_Renderer, nullptr);
  }

  ImGui::Image((ImTextureID)m_GameRenderTarget, viewportSize);

  if (m_SceneState == SceneState::EDIT && m_SelectedEntity) {
    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportSize.x,
                      viewportSize.y);

    glm::mat4 viewMatrix = s_EditorCamera.getViewMatrix();
    glm::mat4 projectionMatrix = s_EditorCamera.getProjectionMatrix();
    glm::mat4 entityWorld = GetWorldMatrix(m_SelectedEntity);
    glm::mat4 mvp = projectionMatrix * viewMatrix * entityWorld;

    if (g_EditColliderMode) {
      if (g_EditColliderMode &&
          m_SelectedEntity->hasComponent<Engine::PolygonColliderComponent>()) {
        auto col =
            m_SelectedEntity->getComponent<Engine::PolygonColliderComponent>();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        for (int i = 0; i < (int)col->vertices.size(); i++) {
          // 1. Calculate Screen Position for rendering handles
          glm::vec4 localPos =
              glm::vec4(col->vertices[i].x + col->offset.x,
                        col->vertices[i].y + col->offset.y, 0.0f, 1.0f);
          glm::vec4 clipPos = mvp * localPos;

          if (clipPos.w <= 0.0f)
            continue;

          glm::vec3 ndc =
              glm::vec3(clipPos.x / clipPos.w, clipPos.y / clipPos.w,
                        clipPos.z / clipPos.w);
          ImVec2 screenPos =
              ImVec2(viewportPos.x + (ndc.x + 1.0f) * 0.5f * viewportSize.x,
                     viewportPos.y + (1.0f - ndc.y) * 0.5f * viewportSize.y);

          float handleRadius = 6.0f;
          float distSq =
              (mousePos.x - screenPos.x) * (mousePos.x - screenPos.x) +
              (mousePos.y - screenPos.y) * (mousePos.y - screenPos.y);
          bool isHovered = distSq < (handleRadius * handleRadius * 4.0f);

          if (isHovered && ImGui::IsMouseDown(0) &&
              s_DraggingVertexIndex == -1) {
            s_DraggingVertexIndex = i;
          }

          // 2. 1-to-1 Dragging Logic
          if (s_DraggingVertexIndex == i) {
            if (ImGui::IsMouseDown(0)) {
              // Convert Current Mouse Position to NDC (-1 to 1)
              float mouseNDC_X =
                  ((mousePos.x - viewportPos.x) / viewportSize.x) * 2.0f - 1.0f;
              float mouseNDC_Y =
                  1.0f - ((mousePos.y - viewportPos.y) / viewportSize.y) * 2.0f;

              // Unproject mouse NDC back to Local Space using Inverse MVP
              glm::vec4 mouseLocalPos =
                  glm::inverse(mvp) *
                  glm::vec4(mouseNDC_X, mouseNDC_Y, ndc.z, 1.0f);
              if (mouseLocalPos.w != 0.0f) {
                mouseLocalPos /= mouseLocalPos.w;

                // Update vertex position (subtracting offset if component uses
                // one)
                col->vertices[i].x = mouseLocalPos.x - col->offset.x;
                col->vertices[i].y = mouseLocalPos.y - col->offset.y;
              }
            }

            if (ImGui::IsMouseReleased(0)) {
              s_DraggingVertexIndex = -1;
            }
          }

          // 3. Visualization
          ImU32 color = (s_DraggingVertexIndex == i)
                            ? IM_COL32(255, 255, 0, 255)
                            : (isHovered ? IM_COL32(255, 200, 0, 255)
                                         : IM_COL32(0, 255, 0, 255));
          drawList->AddCircleFilled(screenPos, handleRadius, color);
          drawList->AddCircle(screenPos, handleRadius + 1.0f,
                              IM_COL32(0, 0, 0, 255));
        }
      }
      if (m_SelectedEntity->hasComponent<Engine::BoxColliderComponent>()) {
        auto col =
            m_SelectedEntity->getComponent<Engine::BoxColliderComponent>();
        glm::mat4 colMatrix =
            glm::translate(entityWorld, glm::vec3(col->offset, 0.0f));
        colMatrix = glm::rotate(colMatrix, glm::radians(col->rotation),
                                glm::vec3(0, 0, 1));
        colMatrix = glm::scale(colMatrix, glm::vec3(col->size, 1.0f));

        if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix),
                                 glm::value_ptr(projectionMatrix),
                                 ImGuizmo::UNIVERSAL, ImGuizmo::LOCAL,
                                 glm::value_ptr(colMatrix))) {
          glm::mat4 rel = glm::inverse(entityWorld) * colMatrix;
          float t[3], r[3], s[3];
          ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(rel), t, r, s);
          col->offset = {t[0], t[1]};
          col->rotation = r[2];
          col->size = {s[0], s[1]};
        }
      }
      // 2. Circle Collider (Universal Gizmo)
      else if (m_SelectedEntity
                   ->hasComponent<Engine::CircleColliderComponent>()) {
        auto col =
            m_SelectedEntity->getComponent<Engine::CircleColliderComponent>();
        glm::mat4 colMatrix =
            glm::translate(entityWorld, glm::vec3(col->offset, 0.0f));
        colMatrix =
            glm::scale(colMatrix, glm::vec3(col->radius, col->radius, 1.0f));

        if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix),
                                 glm::value_ptr(projectionMatrix),
                                 ImGuizmo::UNIVERSAL, ImGuizmo::LOCAL,
                                 glm::value_ptr(colMatrix))) {
          glm::mat4 rel = glm::inverse(entityWorld) * colMatrix;
          float t[3], r[3], s[3];
          ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(rel), t, r, s);
          col->offset = {t[0], t[1]};
          col->radius = (s[0] + s[1]) * 0.5f;
        }
      }

    } else {
      // --- STANDARD TRANSFORM MODE ---
      ImGuizmo::OPERATION op = (currentOperation == GizmoOperation::ROTATE)
                                   ? ImGuizmo::ROTATE
                                   : (currentOperation == GizmoOperation::SCALE
                                          ? ImGuizmo::SCALE
                                          : ImGuizmo::TRANSLATE);
      if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix),
                               glm::value_ptr(projectionMatrix), op,
                               ImGuizmo::WORLD, glm::value_ptr(entityWorld))) {
        auto tr = m_SelectedEntity->getComponent<Engine::TransformComponent>();
        glm::mat4 localMatrix =
            m_SelectedEntity->getParent()
                ? glm::inverse(GetWorldMatrix(m_SelectedEntity->getParent())) *
                      entityWorld
                : entityWorld;
        float t[3], r[3], s[3];
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localMatrix), t, r,
                                              s);
        tr->position = {t[0], t[1]};
        tr->rotation = r[2];
        tr->scale = {s[0], s[1]};
      }
    }
  }

  ImGui::SetCursorScreenPos(ImVec2(viewportPos.x + 10, viewportPos.y + 10));
  if (m_SceneState == SceneState::EDIT) {
    if (ImGui::Button(" ▶ Play "))
      m_SceneState = SceneState::PLAY;
  } else {
    if (ImGui::Button(" ■ Stop "))
      m_SceneState = SceneState::EDIT;
  }

  ImGui::End();
  ImGui::PopStyleVar();
}