#pragma once

#include "../Component.h"
#include <vector>
#include <memory>

namespace Engine
{
    class Entity;
    class InheritanceComponent : public Component
    {
    public:
        Entity *parent = nullptr;
        std::vector<Entity *> children;
        std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<InheritanceComponent>(*this);
        }

        virtual void onShutdown() override;
    };
}
