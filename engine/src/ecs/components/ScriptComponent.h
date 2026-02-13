#pragma once

#include "../Component.h"
#include <string>
#include <sol/sol.hpp>
#include <memory>

namespace Engine {

    class ScriptComponent : public Component {
    public:
        std::string scriptPath;
        bool isInitialized = false;

        sol::environment env;

        ScriptComponent(const std::string& path = "") : scriptPath(path) {}
        std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<ScriptComponent>(*this);
        }
    };
}