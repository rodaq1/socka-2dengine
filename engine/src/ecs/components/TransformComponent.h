#pragma once

#include "../Component.h"
#include "glm/ext/vector_float2.hpp"
#include <memory>
#include <glm/glm.hpp>

namespace Engine {
    /**
     * @brief Komponent, ktory drzi priestorove informacie: poziciu, rotaciu a scale.  
     */
     class TransformComponent : public Component {
    public:
        glm::vec2 position = {0.0f, 0.0f};
        float rotation = 0.0f;
        glm::vec2 scale = {1.0f, 1.0f};

        TransformComponent(glm::vec2 pos = {0.0f, 0.0f}, float rot = 0.0f, glm::vec2 scl = {1.0f, 1.0f}) 
            : position(pos), rotation(rot), scale(scl) {}
            std::unique_ptr<Component> clone() const override
        {
            return std::make_unique<TransformComponent>(*this);
        }
    };
}