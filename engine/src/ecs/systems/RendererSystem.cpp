#include "RendererSystem.h"
#include "../Entity.h"
#include "../components/SpriteComponent.h"
#include "../components/TransformComponent.h"
#include "core/Camera.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"
#include "../components/AnimationComponent.h"
#include "ImGuizmo.h"

namespace Engine
{

  static glm::mat4 GetWorldMatrix(Entity *e)
  {
    if (!e)
      return glm::mat4(1.0f);
    auto tr = e->getComponent<TransformComponent>();
    if (!tr)
      return glm::mat4(1.0f);

    glm::mat4 local = glm::translate(glm::mat4(1.0f), glm::vec3(tr->position.x, tr->position.y, 0.0f));
    local = glm::rotate(local, glm::radians(tr->rotation), glm::vec3(0, 0, 1));
    local = glm::scale(local, glm::vec3(tr->scale.x, tr->scale.y, 1.0f));

    if (e->getParent())
    {
      return GetWorldMatrix(e->getParent()) * local;
    }
    return local;
  }

  RendererSystem::RendererSystem()
  {
    requireComponent<TransformComponent>();
    requireComponent<SpriteComponent>();
    Log::info("renderer system initialized");
  }

  void RendererSystem::update(SDL_Renderer *renderer, const Camera &camera, float targetWidth, float targetHeight, float dt)
  {
    if (!renderer || targetWidth <= 0 || targetHeight <= 0)
      return;

    auto entities = getSystemEntities();
    if (entities.empty())
    {
      static bool loggedOnce = false;
      if (!loggedOnce)
      {
        Log::warn("RendererSystem: No entities found matching components! check if components are added to entities.");
        loggedOnce = true;
      }
      return;
    }

    std::sort(entities.begin(), entities.end(), [](Entity *a, Entity *b)
              {
    auto sA = a->getComponent<SpriteComponent>();
    auto sB = b->getComponent<SpriteComponent>();
    return sA->zIndex < sB->zIndex; });

    glm::mat4 viewProjection = camera.getViewProjectionMatrix();

    glm::mat4 viewportMatrix = glm::mat4(1.0f);
    viewportMatrix = glm::translate(viewportMatrix, glm::vec3(targetWidth * 0.5f, targetHeight * 0.5f, 0.0f));
    viewportMatrix = glm::scale(viewportMatrix, glm::vec3(targetWidth * 0.5f, -targetHeight * 0.5f, 1.0f));

    for (auto entity : entities)
    {

      auto sprite = entity->getComponent<SpriteComponent>();
      if (!sprite)
      {
        Log::warn("Entity " + entity->getName() + " has no SpriteComponent");
        continue;
      }

      auto anim = entity->getComponent<AnimationComponent>();

      if (anim && anim->isPlaying && !anim->currentAnimationName.empty())
      {
        auto it = anim->animations.find(anim->currentAnimationName);
        if (it != anim->animations.end())
        {
          Animation &current = it->second;

          anim->timer += dt;

          if (anim->timer >= current.speed)
          {
            anim->timer = 0.0f;
            anim->currentFrame++;

            if (anim->currentFrame >= current.frameCount)
            {
              if (anim->isLooping)
                anim->currentFrame = 0;
              else
              {
                anim->currentFrame = current.frameCount - 1;
                anim->isPlaying = false;
              }
            }
          }

          // Update sprite sourceRect based on animation
          sprite->sourceRect.y = current.row * sprite->sourceRect.h;
          sprite->sourceRect.x = anim->currentFrame * sprite->sourceRect.w;
        }
      }

      if (sprite->isFixed)
      {
        auto transform = entity->getComponent<TransformComponent>();
        SDL_FRect destRect;
        destRect.x = transform->position.x;
        destRect.y = transform->position.y;
        destRect.w = sprite->sourceRect.w * transform->scale.x;
        destRect.h = sprite->sourceRect.h * transform->scale.y;
        float rotation = -transform->rotation;
        SDL_RenderCopyExF(renderer, sprite->texture, &sprite->sourceRect, &destRect, rotation, nullptr, SDL_FLIP_NONE);
        continue;
      }

      glm::mat4 modelMatrix = GetWorldMatrix(entity);

      float pos[3], rot[3], scale[3];
      ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelMatrix), pos, rot, scale);

      glm::vec4 clipPos = viewProjection * glm::vec4(pos[0], pos[1], pos[2], 1.0f);
      if (std::abs(clipPos.w) < 0.0001f)
        continue;

      glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
      glm::vec4 screenPos = viewportMatrix * glm::vec4(ndcPos, 1.0f);

      float pixelsPerUnitX = std::abs(camera.getProjectionMatrix()[0][0]) * (targetWidth * 0.5f);
      float pixelsPerUnitY = std::abs(camera.getProjectionMatrix()[1][1]) * (targetHeight * 0.5f);

      SDL_FRect destRect;
      destRect.w = std::abs(sprite->sourceRect.w * scale[0] * pixelsPerUnitX);
      destRect.h = std::abs(sprite->sourceRect.h * scale[1] * pixelsPerUnitY);

      destRect.x = screenPos.x - (destRect.w * 0.5f);
      destRect.y = screenPos.y - (destRect.h * 0.5f);

      SDL_SetTextureColorMod(sprite->texture, sprite->color.r, sprite->color.g, sprite->color.b);
      SDL_SetTextureAlphaMod(sprite->texture, sprite->color.a);

      SDL_RendererFlip flip = SDL_FLIP_NONE;
      if (sprite->flipV)
        flip = (SDL_RendererFlip)(flip | SDL_FLIP_VERTICAL);
      if (sprite->flipH)
        flip = (SDL_RendererFlip)(flip | SDL_FLIP_HORIZONTAL);

      SDL_FPoint center = {destRect.w * 0.5f, destRect.h * 0.5f};
      SDL_RenderCopyExF(renderer, sprite->texture, &sprite->sourceRect, &destRect, -rot[2], &center, flip);
    }
  }
} // namespace Engine