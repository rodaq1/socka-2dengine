#include "ScriptSystem.h"
#include "../components/ScriptComponent.h"
#include "../components/TransformComponent.h"
#include "../../core/Log.h"
#include "../../ecs/Entity.h"
#include "core/FileWatcher.h"
#include "ecs/Lua/LuaBridge.h"
#include <glm/glm.hpp>
#include <memory>
#include <filesystem>

namespace Engine {

    ScriptSystem::ScriptSystem() {
        requireComponent<ScriptComponent>();

        m_Lua.open_libraries(
            sol::lib::base,
            sol::lib::package,
            sol::lib::math,
            sol::lib::string
        );

        LuaBridge::Register(m_Lua);

        // FIX: Use a relative path that works in both development and export
        std::string scriptPath = "assets/scripts";

        // Only initialize the watcher if the directory actually exists
        if (std::filesystem::exists(scriptPath)) {
            m_ScriptWatcher = std::make_unique<FileWatcher>(scriptPath);
            m_ScriptWatcher->init();
            Log::info("Script watcher initialized for: " + scriptPath);
        } else {
            Log::warn("Script directory not found for watching: " + scriptPath + ". Hot-reloading disabled.");
            m_ScriptWatcher = nullptr;
        }
    }

    ScriptSystem::~ScriptSystem() {}

    void ScriptSystem::onInit() {
        for (auto entity : getSystemEntities()) {
            initializeEntityScript(entity);
        }
    }

    void ScriptSystem::initializeEntityScript(Entity* entity) {
        auto sc = entity->getComponent<ScriptComponent>();
        if (!sc) return;

        // Ensure the path in the component is also relative or corrected
        sol::load_result loadResult = m_Lua.load_file(sc->scriptPath);
        if (!loadResult.valid()) {
            sol::error err = loadResult;
            Log::error("Failed to load script: " + sc->scriptPath + " | " + err.what());
            return;
        }

        sol::protected_function_result execResult = loadResult();
        if (!execResult.valid()) {
            sol::error err = execResult;
            Log::error("Failed to execute script: " + sc->scriptPath + " | " + err.what());
            return;
        }

        sol::object returned = execResult;
        if (!returned.is<sol::table>()) {
            Log::error("Script did not return a table: " + sc->scriptPath);
            return;
        }

        sol::table scriptTable = returned;
        scriptTable["entity"] = entity;
        
        entityScripts[entity] = scriptTable;
        sc->isInitialized = true;

        sol::protected_function onCreate = scriptTable["OnCreate"];
        if (onCreate.valid()) {
            auto result = onCreate(scriptTable);
            if (!result.valid()) {
                sol::error err = result;
                Log::error("Lua OnCreate Error (" + entity->getName() + "): " + err.what());
            }
        }
    }

    void ScriptSystem::onUpdate(float dt) {
        // Only update watcher if it was successfully created (prevents crash in export)
        if (m_ScriptWatcher) {
            m_ScriptWatcher->update([this](std::string path, FileWatcher::FileStatus status) {
                if (status == FileWatcher::FileStatus::MODIFIED) {
                    Log::warn("File modified: " + path);
                    reloadScript(path);
                }
            });
        }

        for (auto entity : getSystemEntities()) {
            auto it = entityScripts.find(entity);

            if (it == entityScripts.end()) {
                initializeEntityScript(entity);
                it = entityScripts.find(entity);
                if (it == entityScripts.end()) continue;
            }

            sol::table& scriptTable = it->second;
            sol::protected_function updateFunc = scriptTable["OnUpdate"];

            if (updateFunc.valid()) {
                auto result = updateFunc(scriptTable, dt);
                if (!result.valid()) {
                    sol::error err = result;
                    Log::error("Lua Update Error (" + entity->getName() + "): " + err.what());
                }
            }
        }
    }

    void ScriptSystem::reloadScript(const std::string& path) {
        for (auto entity : getSystemEntities()) {
            auto sc = entity->getComponent<ScriptComponent>();

            if (sc && (sc->scriptPath.find(path) != std::string::npos || path.find(sc->scriptPath) != std::string::npos)) {
                Log::info("Reloading script for entity: " + entity->getName());
                
                sc->isInitialized = false; 
                
                auto it = entityScripts.find(entity);
                if (it != entityScripts.end()) {
                    sol::protected_function onDestroy = it->second["OnDestroy"];
                    if (onDestroy.valid()) onDestroy(it->second);
                }

                entityScripts.erase(entity);
                initializeEntityScript(entity);
            }
        }
    }

}