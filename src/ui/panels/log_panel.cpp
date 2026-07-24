#include "ispp/ui/panels/log_panel.h"
#include "ispp/i18n.h"
#include <imgui.h>

namespace ispp::ui {

void LogPanel::log(std::string_view msg) {
    std::scoped_lock lock(Mutex);
    if (Messages.size() < MAX_LOG) {
        Messages.emplace_back(msg);
        NextIdx = Messages.size();
    } else {
        if (!Wrapped)
            Wrapped = true;
        Messages[NextIdx % MAX_LOG] = msg;
        ++NextIdx;
    }
}

void LogPanel::render() {
    if (!ImGui::Begin(_UI("Log"))) {
        ImGui::End();
        return;
    }

    if (ImGui::SmallButton(_UI("Clear"))) {
        clear();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(_UI("Console output (ring buffer)"));

    ImGui::Separator();
    ImGui::BeginChild("log_scroll", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    {
        // Copy under lock: NextIdx grows unbounded so both render
        // loops must wrap via NextIdx % MAX_LOG when the ring buffer
        // overflows, otherwise the unwrapped loop reads past
        // Messages' end (OOB → SIGSEGV).
        RenderCopy.clear();
        RenderCopy.reserve(MAX_LOG);
        std::scoped_lock lock(Mutex);
        if (Wrapped) {
            const std::size_t WRAP = NextIdx % MAX_LOG;
            for (std::size_t i = WRAP; i < MAX_LOG; ++i)
                RenderCopy.push_back(Messages[i]);
            for (std::size_t i = 0; i < WRAP; ++i)
                RenderCopy.push_back(Messages[i]);
        } else {
            for (std::size_t i = 0; i < NextIdx; ++i)
                RenderCopy.push_back(Messages[i]);
        }
    }

    for (const auto &msg : RenderCopy)
        ImGui::TextUnformatted(msg.c_str());

    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();
}

void LogPanel::clear() {
    std::scoped_lock lock(Mutex);
    Messages.clear();
    NextIdx = 0;
    Wrapped = false;
}

} // namespace ispp::ui
