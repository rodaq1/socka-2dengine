#pragma once

#include "../System.h"

namespace Engine {
    class PhysicsSystem : public System {
    public:
        PhysicsSystem();
        virtual ~PhysicsSystem() = default;
        
        void onUpdate(float dt) override;
    };
}