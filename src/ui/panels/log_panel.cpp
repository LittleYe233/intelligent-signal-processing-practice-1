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
        std::scoped_lock lock(Mutex);
        if (Wrapped) {
            for (std::size_t i = NextIdx; i < MAX_LOG; ++i)
                ImGui::TextUnformatted(Messages[i].c_str());
        }
        for (std::size_t i = 0; i < NextIdx; ++i)
            ImGui::TextUnformatted(Messages[i].c_str());
    }

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
