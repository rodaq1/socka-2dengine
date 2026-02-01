#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <map>

namespace Engine {

    /**
     * @brief Manages the loading, storage, and retrieval of textures.
     * Integrates with the FileSystem to resolve project-relative paths.
     */
    class AssetManager {
    private:
        SDL_Renderer* m_Renderer = nullptr;

    public:
        // Static map to allow global access to textures (Shared across scenes)
        static std::map<std::string, SDL_Texture*> m_Textures;

        AssetManager() = default;
        ~AssetManager();

        /**
         * @brief Attaches the SDL renderer for texture creation.
         */
        void init(SDL_Renderer* renderer);

        /**
         * @brief Loads a texture from a path relative to the project root.
         * @param assetId Unique identifier for the texture (e.g., "player_idle")
         * @param relativePath Path relative to project folder (e.g., "assets/player.png")
         */
        void loadTexture(const std::string& assetId, const std::string& relativePath);
        void loadTextureIfMissing(const std::string& assetId, const std::string& relativePath);
        SDL_Texture* getTextureInstance(const std::string& assetId) const;
        static SDL_Texture* getTexture(const std::string& assetId);

        void clearInstanceAssets();
        static void clearAssets();

        void renameTexture(const std::string& oldId, const std::string& newId);
        void removeTexture(const std::string& assetId);
    }; 

}