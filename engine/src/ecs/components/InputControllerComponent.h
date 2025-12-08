#pragma once

#include "../Component.h"
#include "SDL_scancode.h"
#include <SDL2/SDL.h>
#include <map>
#include <string>
#include <vector>

namespace Engine {

    struct InputMapping {
        std::string actionName;
        SDL_Scancode scancode;
    };

    /**
     * @brief Component identifikujuci entitu kontrolovatelnu pouzivatelskym inputom.
     * * Component cita keyboard input a pouziva skalovatelny action-based mapping system.
     */
    class InputControllerComponent : public Component {
    public:
        std::map<std::string, float> parameters = {};

        std::vector<InputMapping> bindings = {};

        InputControllerComponent() = default;

        InputControllerComponent(float initialSpeed) {}
    };
}