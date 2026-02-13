#pragma once

#include "../Component.h"
#include "SDL_scancode.h"
#include <SDL2/SDL.h>
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace Engine
{

    struct InputMapping
    {
        std::string actionName;
        SDL_Scancode scancode;
    };

    /**
     * @brief Component identifikujuci entitu kontrolovatelnu pouzivatelskym inputom.
     * * Component cita keyboard input a pouziva skalovatelny action-based mapping system.
     */
    class InputControllerComponent : public Component
    {
    public:
        std::map<std::string, float> parameters = {
            {"MoveSpeed", 100.0f} // placeholder for testing
        };

        std::vector<InputMapping> mappings;

        bool overrideDefaultSettings = false;

        InputControllerComponent()
        {
            // Default setup
            parameters["MoveSpeed"] = 200.0f;
            overrideDefaultSettings = false;

            mappings.push_back({"MoveUp", SDL_SCANCODE_W});
            mappings.push_back({"MoveDown", SDL_SCANCODE_S});
            mappings.push_back({"MoveLeft", SDL_SCANCODE_A});
            mappings.push_back({"MoveRight", SDL_SCANCODE_D});
        }

        void addMapping(const std::string &action, SDL_Scancode key)
        {
            mappings.push_back({action, key});
        }
        void addParameter(const std::string &name, float value) {
            parameters[name] = value;
        }
        std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<InputControllerComponent>(*this);
        }
    };
}