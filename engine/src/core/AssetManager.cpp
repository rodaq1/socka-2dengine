#include "AssetManager.h"
#include "SDL_image.h"
#include "core/Log.h"
#include "FileSystem.h"

namespace Engine {

std::map<std::string, SDL_Texture*> AssetManager::m_Textures;

AssetManager::~AssetManager() {
    clearInstanceAssets();
}

void AssetManager::init(SDL_Renderer* renderer) {
    if (!renderer) {
        Log::error("SDL_Renderer null. AssetManager init failed.");
        return;
    }
    m_Renderer = renderer;
    Log::info("AssetManager initialized.");
}

void AssetManager::loadTextureIfMissing(const std::string& assetId, const std::string& relativePath) {
    if (m_Textures.find(assetId) != m_Textures.end()) {
        return; // Already loaded, skip to save performance
    }
    
    // If the assetId is the same as the path, we use it for both
    std::string path = relativePath.empty() ? assetId : relativePath;
    loadTexture(assetId, path);
}

void AssetManager::loadTexture(const std::string& assetId, const std::string& relativePath) {
    if (!m_Renderer) {
        Log::error("AssetManager: Cannot load '" + assetId + "' because m_Renderer is NULL. Did you call init()?");
        return;
    }

    // Resolve the project-relative path
    std::filesystem::path resolvedPath = FileSystem::getAbsolutePath(relativePath);
    std::string fullPath = resolvedPath.string();

    // DIAGNOSTIC: Check if file physically exists before SDL touches it
    if (!std::filesystem::exists(resolvedPath)) {
        Log::error("AssetManager: PHYSICAL FILE MISSING at: " + fullPath);
        return;
    }

    SDL_Surface* surface = IMG_Load(fullPath.c_str());
    if (!surface) {
        Log::error("AssetManager: IMG_Load failed for: " + fullPath + " | Error: " + IMG_GetError());
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_Renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
        Log::error("AssetManager: SDL_CreateTextureFromSurface failed for: " + assetId + " | Error: " + SDL_GetError());
        return;
    }

    if (m_Textures.count(assetId)) {
        SDL_DestroyTexture(m_Textures[assetId]);
        Log::warn("AssetManager: Overwriting existing texture asset: " + assetId);
    }

    m_Textures[assetId] = texture;
    Log::info("AssetManager: Successfully loaded [" + assetId + "] from " + fullPath);
}

SDL_Texture* AssetManager::getTextureInstance(const std::string& assetId) const {
    auto it = m_Textures.find(assetId);
    if (it == m_Textures.end()) {
        // Silent fail here is okay as the RendererSystem logs the warning
        return nullptr;
    }
    return it->second;
}

SDL_Texture* AssetManager::getTexture(const std::string& assetId) {
    auto it = m_Textures.find(assetId);
    return it != m_Textures.end() ? it->second : nullptr;
}

void AssetManager::clearInstanceAssets() {
    clearAssets();
}

void AssetManager::clearAssets() {
    for (auto& [id, tex] : m_Textures) {
        if (tex) SDL_DestroyTexture(tex);
    }
    m_Textures.clear();
    Log::info("AssetManager: All textures cleared.");
}

void AssetManager::removeTexture(const std::string& assetId) {
    auto it = m_Textures.find(assetId);
    if (it != m_Textures.end()) {
        if (it->second) {
            SDL_DestroyTexture(it->second);
        }
        m_Textures.erase(it);
        Log::info("Removed texture: " + assetId);
    } else {
        Log::warn("Cannot remove texture. Not found: " + assetId);
    }
}

void AssetManager::renameTexture(const std::string& oldId, const std::string& newId) {
    if (oldId == newId) return;

    auto it = m_Textures.find(oldId);
    if (it != m_Textures.end()) {
        if (m_Textures.count(newId)) {
            Log::warn("Rename target ID already exists, overwriting: " + newId);
            SDL_DestroyTexture(m_Textures[newId]);
        }
        m_Textures[newId] = it->second;
        m_Textures.erase(it);
        Log::info("Renamed texture '" + oldId + "' -> '" + newId + "'");
    } else {
        Log::warn("Cannot rename texture. Old ID not found: " + oldId);
    }
}

} // namespace Engine