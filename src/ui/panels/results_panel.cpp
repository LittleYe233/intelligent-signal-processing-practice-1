#include "ispp/ui/panels/results_panel.h"

#include <imgui.h>

namespace ispp::ui {

void ResultsPanel::render(const std::optional<RunResult> &result) {
    if (!ImGui::Begin("Results")) {
        ImGui::End();
        return;
    }

    if (!result) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1),
                           "No results — run an experiment first.");
        ImGui::End();
        return;
    }

    const auto &res = *result;

    // Total runtime
    ImGui::Text("Total Runtime: %.3f s", res.TotalRuntimeSec);
    ImGui::Separator();

    if (res.PerMetricStats.empty()) {
        ImGui::Text("(no metrics configured)");
        ImGui::End();
        return;
    }

    // Table of metrics
    static constexpr int COLUMNS = 5;
    if (ImGui::BeginTable("metrics_table", COLUMNS,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableSetupColumn("Metric");
        ImGui::TableSetupColumn("Mean");
        ImGui::TableSetupColumn("Std");
        ImGui::TableSetupColumn("Min");
        ImGui::TableSetupColumn("Max");
        ImGui::TableHeadersRow();

        for (const auto &[name, stats] : res.PerMetricStats) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(name.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%.6e", stats.Mean);
            ImGui::TableNextColumn();
            ImGui::Text("%.6e", stats.Std);
            ImGui::TableNextColumn();
            ImGui::Text("%.6e", stats.Min);
            ImGui::TableNextColumn();
            ImGui::Text("%.6e", stats.Max);
        }

        ImGui::EndTable();
    }

    // Last iteration info
    ImGui::Separator();
    ImGui::Text("Last iteration:");
    if (!res.LastPeaks.empty()) {
        for (std::size_t i = 0; i < res.LastPeaks.size(); ++i) {
            ImGui::Text("  Peak %zu: %.3f Hz  (amp = %.4e)", i,
                        res.LastPeaks[i].FrequencyHz,
                        res.LastPeaks[i].Amplitude);
        }
    } else {
        ImGui::Text("  (no peaks detected)");
    }

    ImGui::End();
}

} // namespace ispp::ui
