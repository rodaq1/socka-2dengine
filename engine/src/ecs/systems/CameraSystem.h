#pragma once

#include "../System.h"

namespace Engine {
    
    class Camera; // Forward declaration

    class CameraSystem : public System {
    public:
        CameraSystem();

        void onUpdate(float dt) override {}; // Does nothing, but satisfies virtual function
        void updateCamera(Camera* mainCamera);
    };
}