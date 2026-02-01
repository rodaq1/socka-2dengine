#pragma once

#include "../Component.h"
#include <vector>

namespace Engine {
    class Entity;
    class InheritanceComponent : public Component {
    public:    
        Entity* parent = nullptr;
        std::vector<Entity*> children;

        virtual void onShutdown() override;
    };
}
