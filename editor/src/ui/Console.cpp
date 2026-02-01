#include "Console.h"

// Initialize static members
std::vector<LogMessage> Console::m_Messages;
std::mutex Console::m_Mutex;
bool Console::m_ScrollToBottom = false;

void Console::clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Messages.clear();
}

void Console::render() {
    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Console")) {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Clear")) clear();
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::Separator();

    // Use a child region for scrolling content
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (copy) ImGui::LogToClipboard();

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (const auto& log : m_Messages) {
            ImGui::PushStyleColor(ImGuiCol_Text, log.color);
            ImGui::TextUnformatted(log.text.c_str());
            ImGui::PopStyleColor();
        }
    }

    if (m_ScrollToBottom || (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
        ImGui::SetScrollHereY(1.0f);
        m_ScrollToBottom = false;
    }

    ImGui::EndChild();
    ImGui::End();
}