#pragma once

#include "SDL_scancode.h"
#include "core/Camera.h"
#include "core/Input.h"
#include "core/Log.h"
#include "ecs/Entity.h"
#include "ecs/components/AnimationComponent.h"
#include "ecs/components/BoxColliderComponent.h"
#include "ecs/components/CameraComponent.h"
#include "ecs/components/CircleColliderComponent.h"
#include "ecs/components/InheritanceComponent.h"
#include "ecs/components/InputControllerComponent.h"
#include "ecs/components/PolygonColliderComponent.h"
#include "ecs/components/RigidBodyComponent.h"
#include "ecs/components/SoundComponent.h"
#include "ecs/components/SpriteComponent.h"
#include "ecs/components/TransformComponent.h"
#include "ecs/components/VelocityComponent.h"
#include "sol/raii.hpp"
#include "sol/table.hpp"
#include <glm/glm.hpp>
#include <sol/sol.hpp>

namespace Engine {

/**
 * @brief Centralizovana trieda, ktora handluje C++ to Lua bindingy
 */
class LuaBridge {
public:
  static void Register(sol::state &lua) {
    RegisterCore(lua);
    RegisterMath(lua);
    RegisterECS(lua);
    RegisterKeycodes(lua);
    Log::info("registered Lua bindings");
  }

private:
  static void RegisterCore(sol::state &lua) {
    auto log = lua["Log"].get_or_create<sol::table>();
    log["info"] = [](const std::string &msg) { Log::info("[Lua] " + msg); };
    log["warn"] = [](const std::string &msg) { Log::warn("[Lua] " + msg); };
    log["error"] = [](const std::string &msg) { Log::error("[Lua] " + msg); };

    auto input = lua["Input"].get_or_create<sol::table>();

    input["isKeyDown"] = [](SDL_Scancode scancode) {
      return Engine::Input::isKeyDown(scancode);
    };
    input["isKeyPressed"] = [](SDL_Scancode scancode) {
      return Engine::Input::isKeyPressed(scancode);
    };
    input["isKeyReleased"] = [](SDL_Scancode scancode) {
      return Engine::Input::isKeyReleased(scancode);
    };
    input["getMousePosition"] = []() {
      SDL_Point pos = Engine::Input::getMousePosition();
      return glm::vec2(static_cast<float>(pos.x), static_cast<float>(pos.y));
    };
    input["isMouseButtonDown"] = [](int button) {
      return Engine::Input::isMouseButtonDown(button);
    };
    input["isMouseButtonPressed"] = [](int button) {
      return Engine::Input::isMouseButtonPressed(button);
    };
    input["isMouseButtonReleased"] = [](int button) {
      return Engine::Input::isMouseButtonReleased(button);
    };
  }

  static void RegisterKeycodes(sol::state &lua) {
    lua["KEY_UNKNOWN"] = SDL_SCANCODE_UNKNOWN;
    lua["KEY_A"] = SDL_SCANCODE_A;
    lua["KEY_B"] = SDL_SCANCODE_B;
    lua["KEY_C"] = SDL_SCANCODE_C;
    lua["KEY_D"] = SDL_SCANCODE_D;
    lua["KEY_E"] = SDL_SCANCODE_E;
    lua["KEY_F"] = SDL_SCANCODE_F;
    lua["KEY_G"] = SDL_SCANCODE_G;
    lua["KEY_H"] = SDL_SCANCODE_H;
    lua["KEY_I"] = SDL_SCANCODE_I;
    lua["KEY_J"] = SDL_SCANCODE_J;
    lua["KEY_K"] = SDL_SCANCODE_K;
    lua["KEY_L"] = SDL_SCANCODE_L;
    lua["KEY_M"] = SDL_SCANCODE_M;
    lua["KEY_N"] = SDL_SCANCODE_N;
    lua["KEY_O"] = SDL_SCANCODE_O;
    lua["KEY_P"] = SDL_SCANCODE_P;
    lua["KEY_Q"] = SDL_SCANCODE_Q;
    lua["KEY_R"] = SDL_SCANCODE_R;
    lua["KEY_S"] = SDL_SCANCODE_S;
    lua["KEY_T"] = SDL_SCANCODE_T;
    lua["KEY_U"] = SDL_SCANCODE_U;
    lua["KEY_V"] = SDL_SCANCODE_V;
    lua["KEY_W"] = SDL_SCANCODE_W;
    lua["KEY_X"] = SDL_SCANCODE_X;
    lua["KEY_Y"] = SDL_SCANCODE_Y;
    lua["KEY_Z"] = SDL_SCANCODE_Z;

    lua["KEY_1"] = SDL_SCANCODE_1;
    lua["KEY_2"] = SDL_SCANCODE_2;
    lua["KEY_3"] = SDL_SCANCODE_3;
    lua["KEY_4"] = SDL_SCANCODE_4;
    lua["KEY_5"] = SDL_SCANCODE_5;
    lua["KEY_6"] = SDL_SCANCODE_6;
    lua["KEY_7"] = SDL_SCANCODE_7;
    lua["KEY_8"] = SDL_SCANCODE_8;
    lua["KEY_9"] = SDL_SCANCODE_9;
    lua["KEY_0"] = SDL_SCANCODE_0;

    lua["KEY_RETURN"] = SDL_SCANCODE_RETURN;
    lua["KEY_ESCAPE"] = SDL_SCANCODE_ESCAPE;
    lua["KEY_BACKSPACE"] = SDL_SCANCODE_BACKSPACE;
    lua["KEY_TAB"] = SDL_SCANCODE_TAB;
    lua["KEY_SPACE"] = SDL_SCANCODE_SPACE;
    lua["KEY_MINUS"] = SDL_SCANCODE_MINUS;
    lua["KEY_EQUALS"] = SDL_SCANCODE_EQUALS;
    lua["KEY_LEFTBRACKET"] = SDL_SCANCODE_LEFTBRACKET;
    lua["KEY_RIGHTBRACKET"] = SDL_SCANCODE_RIGHTBRACKET;
    lua["KEY_BACKSLASH"] = SDL_SCANCODE_BACKSLASH;
    lua["KEY_SEMICOLON"] = SDL_SCANCODE_SEMICOLON;
    lua["KEY_APOSTROPHE"] = SDL_SCANCODE_APOSTROPHE;
    lua["KEY_GRAVE"] = SDL_SCANCODE_GRAVE;
    lua["KEY_COMMA"] = SDL_SCANCODE_COMMA;
    lua["KEY_PERIOD"] = SDL_SCANCODE_PERIOD;
    lua["KEY_SLASH"] = SDL_SCANCODE_SLASH;

    lua["KEY_CAPSLOCK"] = SDL_SCANCODE_CAPSLOCK;
    lua["KEY_F1"] = SDL_SCANCODE_F1;
    lua["KEY_F2"] = SDL_SCANCODE_F2;
    lua["KEY_F3"] = SDL_SCANCODE_F3;
    lua["KEY_F4"] = SDL_SCANCODE_F4;
    lua["KEY_F5"] = SDL_SCANCODE_F5;
    lua["KEY_F6"] = SDL_SCANCODE_F6;
    lua["KEY_F7"] = SDL_SCANCODE_F7;
    lua["KEY_F8"] = SDL_SCANCODE_F8;
    lua["KEY_F9"] = SDL_SCANCODE_F9;
    lua["KEY_F10"] = SDL_SCANCODE_F10;
    lua["KEY_F11"] = SDL_SCANCODE_F11;
    lua["KEY_F12"] = SDL_SCANCODE_F12;

    lua["KEY_PRINTSCREEN"] = SDL_SCANCODE_PRINTSCREEN;
    lua["KEY_SCROLLLOCK"] = SDL_SCANCODE_SCROLLLOCK;
    lua["KEY_PAUSE"] = SDL_SCANCODE_PAUSE;
    lua["KEY_INSERT"] = SDL_SCANCODE_INSERT;
    lua["KEY_HOME"] = SDL_SCANCODE_HOME;
    lua["KEY_PAGEUP"] = SDL_SCANCODE_PAGEUP;
    lua["KEY_DELETE"] = SDL_SCANCODE_DELETE;
    lua["KEY_END"] = SDL_SCANCODE_END;
    lua["KEY_PAGEDOWN"] = SDL_SCANCODE_PAGEDOWN;
    lua["KEY_RIGHT"] = SDL_SCANCODE_RIGHT;
    lua["KEY_LEFT"] = SDL_SCANCODE_LEFT;
    lua["KEY_DOWN"] = SDL_SCANCODE_DOWN;
    lua["KEY_UP"] = SDL_SCANCODE_UP;

    lua["KEY_NUMLOCKCLEAR"] = SDL_SCANCODE_NUMLOCKCLEAR;
    lua["KEY_KP_DIVIDE"] = SDL_SCANCODE_KP_DIVIDE;
    lua["KEY_KP_MULTIPLY"] = SDL_SCANCODE_KP_MULTIPLY;
    lua["KEY_KP_MINUS"] = SDL_SCANCODE_KP_MINUS;
    lua["KEY_KP_PLUS"] = SDL_SCANCODE_KP_PLUS;
    lua["KEY_KP_ENTER"] = SDL_SCANCODE_KP_ENTER;
    lua["KEY_KP_1"] = SDL_SCANCODE_KP_1;
    lua["KEY_KP_2"] = SDL_SCANCODE_KP_2;
    lua["KEY_KP_3"] = SDL_SCANCODE_KP_3;
    lua["KEY_KP_4"] = SDL_SCANCODE_KP_4;
    lua["KEY_KP_5"] = SDL_SCANCODE_KP_5;
    lua["KEY_KP_6"] = SDL_SCANCODE_KP_6;
    lua["KEY_KP_7"] = SDL_SCANCODE_KP_7;
    lua["KEY_KP_8"] = SDL_SCANCODE_KP_8;
    lua["KEY_KP_9"] = SDL_SCANCODE_KP_9;
    lua["KEY_KP_0"] = SDL_SCANCODE_KP_0;
    lua["KEY_KP_PERIOD"] = SDL_SCANCODE_KP_PERIOD;

    lua["KEY_APPLICATION"] = SDL_SCANCODE_APPLICATION;
    lua["KEY_POWER"] = SDL_SCANCODE_POWER;
    lua["KEY_KP_EQUALS"] = SDL_SCANCODE_KP_EQUALS;
    lua["KEY_F13"] = SDL_SCANCODE_F13;
    lua["KEY_F14"] = SDL_SCANCODE_F14;
    lua["KEY_F15"] = SDL_SCANCODE_F15;
    lua["KEY_F16"] = SDL_SCANCODE_F16;
    lua["KEY_F17"] = SDL_SCANCODE_F17;
    lua["KEY_F18"] = SDL_SCANCODE_F18;
    lua["KEY_F19"] = SDL_SCANCODE_F19;
    lua["KEY_F20"] = SDL_SCANCODE_F20;
    lua["KEY_F21"] = SDL_SCANCODE_F21;
    lua["KEY_F22"] = SDL_SCANCODE_F22;
    lua["KEY_F23"] = SDL_SCANCODE_F23;
    lua["KEY_F24"] = SDL_SCANCODE_F24;
    lua["KEY_EXECUTE"] = SDL_SCANCODE_EXECUTE;
    lua["KEY_HELP"] = SDL_SCANCODE_HELP;
    lua["KEY_MENU"] = SDL_SCANCODE_MENU;
    lua["KEY_SELECT"] = SDL_SCANCODE_SELECT;
    lua["KEY_STOP"] = SDL_SCANCODE_STOP;
    lua["KEY_AGAIN"] = SDL_SCANCODE_AGAIN;
    lua["KEY_UNDO"] = SDL_SCANCODE_UNDO;
    lua["KEY_CUT"] = SDL_SCANCODE_CUT;
    lua["KEY_COPY"] = SDL_SCANCODE_COPY;
    lua["KEY_PASTE"] = SDL_SCANCODE_PASTE;
    lua["KEY_FIND"] = SDL_SCANCODE_FIND;
    lua["KEY_MUTE"] = SDL_SCANCODE_MUTE;
    lua["KEY_VOLUMEUP"] = SDL_SCANCODE_VOLUMEUP;
    lua["KEY_VOLUMEDOWN"] = SDL_SCANCODE_VOLUMEDOWN;
    lua["KEY_KP_COMMA"] = SDL_SCANCODE_KP_COMMA;
    lua["KEY_KP_EQUALSAS400"] = SDL_SCANCODE_KP_EQUALSAS400;
  }
  static void RegisterMath(sol::state &lua) {
    lua.new_usertype<glm::vec2>(
        "vec2", sol::constructors<glm::vec2(), glm::vec2(float, float)>(), "x",
        &glm::vec2::x, "y", &glm::vec2::y, sol::meta_function::addition,
        [](const glm::vec2 &a, const glm::vec2 &b) { return a + b; },
        sol::meta_function::multiplication,
        [](const glm::vec2 &a, float b) { return a * b; });
  }
  static void RegisterECS(sol::state &lua) {
    //transform
    lua.new_usertype<TransformComponent>(
        "Transform", "position", &TransformComponent::position, "rotation",
        &TransformComponent::rotation, "scale", &TransformComponent::scale);

    //rigidbody
    lua.new_usertype<RigidBodyComponent>(
        "RigidBody", "velocity", &RigidBodyComponent::velocity, "acceleration",
        &RigidBodyComponent::acceleration, "mass", &RigidBodyComponent::mass,
        "linearDrag", &RigidBodyComponent::linearDrag, "gravityScale",
        &RigidBodyComponent::gravityScale, "bodyType",
        &RigidBodyComponent::bodyType, "addForce",
        &RigidBodyComponent::addForce);

    /**
     * COLLIDERS
     * Box collider
     */
    lua.new_usertype<BoxColliderComponent>(
        "BoxCollider", "size", &BoxColliderComponent::size, "offset",
        &BoxColliderComponent::offset, "layer", &BoxColliderComponent::layer,
        "mask", &BoxColliderComponent::mask, "isTrigger",
        &BoxColliderComponent::isTrigger, "isStatic",
        "onTriggerEnter", &BoxColliderComponent::onTriggerEnter,
        &BoxColliderComponent::isStatic);

    //polygon collider
    lua.new_usertype<PolygonColliderComponent>(
        "PolygonCollider",
        "vertices", &PolygonColliderComponent::vertices,
        "offset", &PolygonColliderComponent::offset,
        "rotation", &PolygonColliderComponent::rotation,
        "layer", &PolygonColliderComponent::layer,
        "mask", &PolygonColliderComponent::mask,
        "isTrigger", &PolygonColliderComponent::isTrigger,
        "isStatic", &PolygonColliderComponent::isStatic,
        "onTriggerEnter", &PolygonColliderComponent::onTriggerEnter,
        "addVertex", &PolygonColliderComponent::addVertex,
        "clearVertices", &PolygonColliderComponent::clearVertices
    );

    //circle collider
    lua.new_usertype<CircleColliderComponent>(
        "CircleCollider",
        "offset", &CircleColliderComponent::offset,
        "radius", &CircleColliderComponent::radius,
        "layer", &CircleColliderComponent::layer,
        "onTriggerEnter", &CircleColliderComponent::onTriggerEnter,
        "mask", &CircleColliderComponent::mask,
        "isTrigger", &CircleColliderComponent::isTrigger,
        "isStatic", &CircleColliderComponent::isStatic
    );
    
    //animation
    lua.new_usertype<AnimationComponent>(
        "AnimationComponent", "currentFrame", &AnimationComponent::currentFrame,
        "currentAnimationName", &AnimationComponent::currentAnimationName,
        "isLooping", &AnimationComponent::isLooping, "isPlaying",
        &AnimationComponent::isPlaying, "timer", &AnimationComponent::timer,
        "addAnimation", &AnimationComponent::addAnimation, "play",
        &AnimationComponent::play, "stop", &AnimationComponent::stop);

    // camera class
    lua.new_usertype<Engine::Camera>(
        "Camera",
        "position", sol::property(&Engine::Camera::getPosition, &Engine::Camera::setPosition),
        "rotation", sol::property(&Engine::Camera::getRotation, &Engine::Camera::setRotation),
        "orthographicSize", sol::property(&Engine::Camera::getOrthographicSize, &Engine::Camera::setOrthographicSize),
        "aspectRatio", sol::property(&Engine::Camera::getAspectRatio, &Engine::Camera::setAspectRatio)
    );

    //camera component
    lua.new_usertype<Engine::CameraComponent>(
        "CameraComponent",
        "isPrimary", &Engine::CameraComponent::isPrimary
    );
      
    //input controller
    lua.new_usertype<InputControllerComponent>(
        "InputControllerComponent", "getParameter",
        [](InputControllerComponent &c, const std::string &name) -> float {
          auto it = c.parameters.find(name);
          return it != c.parameters.end() ? it->second : 0.0f;
        },
        "setParameter",
        [](InputControllerComponent &c, const std::string &name, float value) {
          c.parameters[name] = value;
        },
        "mappings", &InputControllerComponent::mappings, "addMapping",
        [](InputControllerComponent &c, const std::string &action, int key) {
          c.addMapping(action, static_cast<SDL_Scancode>(key));
        },
        "isActionDown",
        [](InputControllerComponent &c, const std::string &action) {
          for (auto &mapping : c.mappings) {
            if (mapping.actionName == action) {
              if (Input::isKeyDown(mapping.scancode)) {
                return true;
              }
            }
          }
          return false;
        });
    
    //sprite component
    lua.new_usertype<SpriteComponent>(
        "Sprite", "assetId", 
        &SpriteComponent::assetId, "visible", 
        &SpriteComponent::visible, "zIndex", 
        &SpriteComponent::zIndex
      );

    //SOUND 
    lua.new_usertype<SoundComponent>(
        "Sound", 
        "volume", &SoundComponent::volume,
          "play", &SoundComponent::play, 
          "stop", &SoundComponent::stop
      );

    //CHILD parent relations
    lua.new_usertype<InheritanceComponent>(
      "Inheritance", 
      "parent", &InheritanceComponent::parent, 
      "children", &InheritanceComponent::children
    );

    //velocity component
    lua.new_usertype<VelocityComponent>(
        "Velocity",
        "velocity", &VelocityComponent::velocity
    );
    
    lua.new_usertype<InputMapping>("InputMapping", "actionName",
                                   &InputMapping::actionName, "scancode",
                                   &InputMapping::scancode);

    lua.new_usertype<Entity>(
        "Entity", 
        "getName", &Entity::getName, 
        "getTransform", [](Entity &e) { return e.getComponent<TransformComponent>(); },
        "getVelocity", [](Entity &e) { return e.getComponent<VelocityComponent>(); },
        "getSprite", [](Entity &e) { return e.getComponent<SpriteComponent>(); },
        "getRigidbody", [](Entity &e) { return e.getComponent<RigidBodyComponent>(); },
        "getBoxCollider", [](Entity &e) { return e.getComponent<BoxColliderComponent>(); },
        "getCircleCollider", [](Entity &e) { return e.getComponent<CircleColliderComponent>(); },
        "getPolygonCollider", [](Entity &e) { return e.getComponent<PolygonColliderComponent>(); },
        "getSound", [](Entity &e) { return e.getComponent<SoundComponent>(); },
        "getInheritance", [](Entity &e) { return e.getComponent<InheritanceComponent>(); }
    );
  }
};
} // namespace Engine