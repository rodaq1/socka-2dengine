#pragma once

#include "../System.h"
#include "core/FileWatcher.h"
#include "core/Log.h"
#include "sol/table.hpp"
#include <sol/sol.hpp>
#include <unordered_map>

namespace Engine{

    class Project;

    class ScriptSystem : public System {
    public:
        ScriptSystem(Project* project);
        ~ScriptSystem();

        void onInit() override;
        void onUpdate(float dt) override;
        void reloadScript(const std::string& path);


    private:
        sol::state m_Lua;

        std::unordered_map<class Entity*, sol::table> entityScripts;

        std::unique_ptr<FileWatcher> m_ScriptWatcher;

        void initializeEntityScript(class Entity* entity);


        void verifyScriptFunctionalitly(sol::state& lua, const std::string& scriptPath) {
            Log::info("Starting verification for script: " + scriptPath);

            sol::protected_function_result loadResult = lua.safe_script_file(scriptPath, sol::script_pass_on_error);

            if (!loadResult.valid()) {
                sol::error err = loadResult;
                Log::error("Script Syntax/Load Error: " + std::string(err.what()));
                return;
            }
        
            if (loadResult.get_type() != sol::type::table) {
                Log::error("Script Validation Failed: " + scriptPath + " must return a table.");
                return;
            }
        
            sol::table scriptTable = loadResult;
            Log::info("Table signature detected. Checking for lifecycle hooks...");
        
            bool hasOnCreate = scriptTable["OnCreate"].is<sol::function>();
            bool hasOnUpdate = scriptTable["OnUpdate"].is<sol::function>();
        
            if (!hasOnCreate) {
                Log::warn("Script " + scriptPath + " is missing 'OnCreate'. This may be intentional.");
            }

            if (!hasOnUpdate) {
                Log::warn("Script " + scriptPath + " is missing 'OnUpdate'. This script won't run logic per-frame.");
            }
        
            if (hasOnCreate) {
                Log::info("OnCreate lifecycle hook detected."); 
            }
        
            Log::info("Verification complete for: " + scriptPath);
        }
    };
}

