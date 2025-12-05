#include "AssetManager.h"
#include "SDL_error.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "core/Log.h"
#include <SDL2/SDL_image.h>
#include <string>

namespace Engine { 
    AssetManager::AssetManager() {
        Log::info("AssetManager initializovany.");
    }

    AssetManager::~AssetManager() {
        clearAssets();
        Log::info("AssetManager zniceny.");
    }

    void AssetManager::init(SDL_Renderer* renderer) {
        if (!renderer) {
            Log::error("SDL_Renderer null. Neda sa initnut AssetManager.");
            return;
        }
        m_Renderer = renderer;
    }

    void AssetManager::loadTexture(const std::string& assetId, const std::string& filePath) {
        if (!m_Renderer) {
            Log::error("Textura sa neda loadnut. AssetManager nebol initializovany s rendererom.");
        }

        SDL_Surface* surface = IMG_Load(filePath.c_str());
        if (!surface) {
            Log::error("Zlyhalo nacitanie imagu: " + filePath + ". SDL_image error: " + IMG_GetError());
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
        SDL_FreeSurface(surface);

        if (!texture) {
            Log::error("Nepodarilo sa vytvorit texturu: " + std::string(SDL_GetError()));
            return;
        }

        if (m_Textures.count(assetId)) {
            Log::warn("Asset ID '" + assetId + "' uz existuje. Prepisujem.");
            SDL_DestroyTexture(m_Textures[assetId]);
        }

        m_Textures[assetId] = texture;
        Log::info("Nacitana textura: " + assetId + " z " + filePath);
    }

    SDL_Texture* AssetManager::getTexture(const std::string& assetId) const {
        if (m_Textures.count(assetId)) {
            return m_Textures.at(assetId);
        }
        Log::warn("Textura s id '" + assetId + "' nebola najdena.");
        return nullptr;
    }

    void AssetManager::clearAssets() {
        for (auto const& [key, val] : m_Textures) {
            SDL_DestroyTexture(val);
        }
        m_Textures.clear();
        Log::info("Vsetky assety odstranene z pamate.");
    }
}