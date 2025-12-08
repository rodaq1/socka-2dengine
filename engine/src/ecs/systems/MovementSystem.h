#pragma once

#include "../System.h"

namespace Engine {
    /**
     * @brief System responzibilny za updatovanie pozicie entit podla ich vektoru pohybu.
     * * Vyzaduje TransformComponent and VelocityComponent.
     */
    class MovementSystem : public System {
    public:
        MovementSystem();

        /**
         * @brief Updatuje poziciu vsetkych relevantnych entit podla deltaTime.
         * @param dt Cas ktory ubehol od posledneho framu (deltaTime)
         */
         void onUpdate(float dt) override;
    };
}