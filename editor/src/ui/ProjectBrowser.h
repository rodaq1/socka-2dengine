#pragma once

#include "core/Project.h"
#include <string>
#include <vector>

class ProjectBrowser {
    public:
        ProjectBrowser() = default;
        ~ProjectBrowser() = default;
        
        Engine::Project* render();
        void refreshProjectList(const std::string& directory);


    private:
        std::string m_SearchPath = "./projects";
        std::vector<Engine::Project> m_FoundProjects;
        int m_SelectedIndex = -1;

        char m_NewProjectName[128] = "NewProject";
        char m_NewProjectPath[512] = "./projects";

        Engine::Project* createNewProject();
        void removeProject(const Engine::Project& project);
    };
