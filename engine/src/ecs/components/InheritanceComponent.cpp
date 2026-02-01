#include "InheritanceComponent.h"
#include "../Entity.h"

namespace Engine {
    void InheritanceComponent::onShutdown() {
        for (auto& child : children) {
            child->shutdown();
        }
    }
}