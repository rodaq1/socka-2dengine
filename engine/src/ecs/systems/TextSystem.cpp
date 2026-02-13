#include <SDL.h>
#include <SDL_ttf.h>

#include "imgui.h"
#include "ImGuizmo.h"

#include "TextSystem.h"
#include "../components/TextComponent.h"
#include "../components/TransformComponent.h"
#include "../Entity.h"

#include "core/Camera.h"
#include "core/FontManager.h"
#include "core/Font.h"
#include "core/Log.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Engine
{

static glm::mat4 GetWorldMatrix(Entity *e)
{
    if (!e)
        return glm::mat4(1.0f);

    auto tr = e->getComponent<TransformComponent>();
    if (!tr)
        return glm::mat4(1.0f);

    glm::mat4 local = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(tr->position.x, tr->position.y, 0.0f));
    local = glm::rotate(local,
                        glm::radians(tr->rotation),
                        glm::vec3(0, 0, 1));
    local = glm::scale(local,
                       glm::vec3(tr->scale.x, tr->scale.y, 1.0f));

    if (e->getParent())
        return GetWorldMatrix(e->getParent()) * local;

    return local;
}

TextSystem::TextSystem()
{
    requireComponent<TransformComponent>();
    requireComponent<TextComponent>();
    Log::info("Text system initialized");
}

TextSystem::~TextSystem()
{
    for (auto &[_, data] : m_TextData)
    {
        if (data.texture)
            SDL_DestroyTexture(data.texture);
    }
}

void TextSystem::update(SDL_Renderer *renderer,
                        const Camera &camera,
                        float targetWidth,
                        float targetHeight,
                        Project *project)
{
    if (!renderer || targetWidth <= 0 || targetHeight <= 0)
        return;

    auto entities = getSystemEntities();
    if (entities.empty())
        return;

    // Sort by Z index
    std::sort(entities.begin(), entities.end(),
              [](Entity *a, Entity *b)
              {
                  return a->getComponent<TextComponent>()->zIndex <
                         b->getComponent<TextComponent>()->zIndex;
              });

    glm::mat4 viewProjection = camera.getViewProjectionMatrix();

    glm::mat4 viewportMatrix(1.0f);
    viewportMatrix = glm::translate(viewportMatrix,
                                    glm::vec3(targetWidth * 0.5f, targetHeight * 0.5f, 0.0f));
    viewportMatrix = glm::scale(viewportMatrix,
                                glm::vec3(targetWidth * 0.5f, -targetHeight * 0.5f, 1.0f));

    for (auto entity : entities)
    {
        auto text = entity->getComponent<TextComponent>();
        auto transform = entity->getComponent<TransformComponent>();
        if (!text || !transform)
            continue;

        // Load font if missing
        if (!text->font && !text->fontPath.empty())
        {
            text->font = FontManager::LoadFont(text->fontPath, text->fontSize, project);
            if (!text->font)
            {
                Log::error(("TextSystem: Failed to load font '" + text->fontPath + "' size " +
                            std::to_string(text->fontSize) + " for entity " + entity->getName()));
                continue;
            }
            text->dirty = true;
        }

        auto &renderData = m_TextData[entity];

        // Rebuild texture if dirty
        if (text->dirty)
        {
            if (renderData.texture)
            {
                SDL_DestroyTexture(renderData.texture);
                renderData.texture = nullptr;
            }

            if (!text->font)
            {
                Log::error(("TextSystem: Cannot render entity '" + entity->getName() +
                            "' because font is null."));
                text->dirty = false;
                continue;
            }

            SDL_Surface *surface = TTF_RenderUTF8_Blended(text->font->get(),
                                                          text->text.c_str(),
                                                          text->color);
            if (!surface)
            {
                Log::error(("TextSystem: Failed to create surface for entity '" +
                            entity->getName() + "': " + TTF_GetError()));
                text->dirty = false;
                continue;
            }

            renderData.texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (!renderData.texture)
            {
                Log::error(("TextSystem: Failed to create texture for entity '" +
                            entity->getName() + "': " + SDL_GetError()));
                SDL_FreeSurface(surface);
                text->dirty = false;
                continue;
            }

            renderData.width = surface->w;
            renderData.height = surface->h;
            SDL_FreeSurface(surface);

            text->dirty = false;
        }

        if (!renderData.texture)
            continue;

        // --------------------------
        // FIXED (UI text)
        // --------------------------
        if (text->isFixed)
        {
            SDL_FRect dst;
            dst.x = transform->position.x;
            dst.y = transform->position.y;
            dst.w = renderData.width * transform->scale.x;
            dst.h = renderData.height * transform->scale.y;

            SDL_RenderCopyExF(renderer,
                              renderData.texture,
                              nullptr,
                              &dst,
                              -transform->rotation,
                              nullptr,
                              SDL_FLIP_NONE);
            continue;
        }

        // --------------------------
        // WORLD SPACE
        // --------------------------
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

        SDL_FRect dst;
        dst.w = std::abs(renderData.width * scale[0] * pixelsPerUnitX);
        dst.h = std::abs(renderData.height * scale[1] * pixelsPerUnitY);
        dst.x = screenPos.x - (dst.w * 0.5f);
        dst.y = screenPos.y - (dst.h * 0.5f);

        SDL_FPoint center = {dst.w * 0.5f, dst.h * 0.5f};

        SDL_RenderCopyExF(renderer,
                          renderData.texture,
                          nullptr,
                          &dst,
                          -rot[2],
                          &center,
                          SDL_FLIP_NONE);
    }
}

} // namespace Engine
