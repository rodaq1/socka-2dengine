#pragma once
#include <imgui.h>
#include <mutex>
#include <string>
#include <vector>


    struct LogMessage {
        std::string text;
        ImVec4 color;
    };

    using namespace ImGui;    
    
    
    class Console {
    private:
        static std::vector<LogMessage> m_Messages;
        static std::mutex m_Mutex;
        static bool m_ScrollToBottom;
        static Console& getInstance() {
            static Console instance;
            return instance;
        }

    public:
        static void addLog(const std::string_view message, ImVec4 color = ImVec4(1, 1, 1, 1)) {
        std::lock_guard<std::mutex> lock(getInstance().m_Mutex);
        
        getInstance().m_Messages.push_back({ std::string(message), color });

        if (getInstance().m_Messages.size() > 500) {
            getInstance().m_Messages.erase(getInstance().m_Messages.begin());
        }
        
        getInstance().m_ScrollToBottom = true;
    }

        static void clear();

        static void render();
    };

