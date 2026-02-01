#pragma once

#include "../System.h"

namespace Engine {

    /**
     * @brief system responzibilny za citanie user inputu a modifyovanie pohybovych hodnot kontrolovatelnych entit.
     * * Vyzaduje InputControllerComponent. Vyuziva sa ako default movement logic maker pokial to nechceme robit cez script
     */
    class InputSystem : public System {
    public:
        InputSystem();

        /**
         * @brief Cita input state a aktualizuje pohybove hodnoty kontrolovatelnych entit.
         */
        void onUpdate(float dt) override;
    };
}