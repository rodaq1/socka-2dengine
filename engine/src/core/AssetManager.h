#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <map>
#include "SDL_render.h"

namespace Engine {
    /**
     * @brief Centralny service na loadovanie, managovanie a retrievovanie hernych assetov.
     * Vyuziva SDL_Texture.
     */
    class AssetManager {
    private:
        SDL_Renderer* m_Renderer = nullptr;
        // Mapa na storovanie pointrov na textury, identifikovane stringovymi klucmi
    public:
        std::map<std::string, SDL_Texture*> m_Textures;

        AssetManager();
        ~AssetManager();

        /**
         * @brief initializer AssetManageru so SDL rendererom.
         * @param renderer Globalna SDL_Renderer instancia na vytvaranie textur.
         */
        void init(SDL_Renderer* renderer);

        /**
         * @brief Loadne texturu zo suboru, zassociuje ju s IDckom a ulozi.
         * @param assetId unikatny identifikator assetu 
         * @param filePath pathka ku image filu.
         */
        void loadTexture(const std::string& assetId, const std::string& filePath);

        /**
         * @brief ziska uz nacitanu texturu podla ID.
         * @param assetId unikatny identifikator nacitanej textury
         * @return SDL_Texture* pointer na requestovanu texturu. Nullptr ak sa nenajde.
         */
        SDL_Texture* getTexture(const std::string& assetId) const;

        /**
         * @brief Znici a clearne vsetky loadnute assety z pamate.
         */
        void clearAssets();
    };
}