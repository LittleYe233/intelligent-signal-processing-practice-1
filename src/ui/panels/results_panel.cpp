#include "ispp/ui/panels/results_panel.h"
#include "ispp/i18n.h"
#include <imgui.h>

namespace ispp::ui {

void ResultsPanel::render(const std::optional<RunResult> &result) {
    if (!ImGui::Begin(_UI("Results & Metrics"))) {
        ImGui::End();
        return;
    }

    if (!result) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1),
                           _UI("No data — run an experiment first."));
        ImGui::End();
        return;
    }

    const auto &res = *result;

    // Total runtime
    ImGui::Text(_UI("Total Runtime: %.3f s"), res.TotalRuntimeSec);
    ImGui::Separator();

    if (res.Metrics.empty()) {
        ImGui::Text(_UI("(no metrics configured)"));
        ImGui::Separator();
    } else {
        // Sort metrics into table-display vs single-value
        bool has_table = false;
        bool has_single = false;
        for (const auto &mr : res.Metrics) {
            if (mr.MetricObj->showDistribution())
                has_table = true;
            else
                has_single = true;
        }

        // --- Table: metrics with full distribution ---
        if (has_table) {
            static constexpr int COLUMNS = 5;
            if (ImGui::BeginTable("metrics_table", COLUMNS,
                                  ImGuiTableFlags_Borders |
                                      ImGuiTableFlags_RowBg |
                                      ImGuiTableFlags_SizingStretchSame)) {
                ImGui::TableSetupColumn(_UI("Metric"));
                ImGui::TableSetupColumn(_UI("Mean"));
                ImGui::TableSetupColumn(_UI("Std"));
                ImGui::TableSetupColumn(_UI("Min"));
                ImGui::TableSetupColumn(_UI("Max"));
                ImGui::TableHeadersRow();

                for (const auto &mr : res.Metrics) {
                    if (!mr.MetricObj->showDistribution())
                        continue;
                    const auto &stats = mr.Stats;
                    const std::string NAME(mr.MetricObj->name());
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(_UI(NAME.c_str()));
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(
                        mr.MetricObj->format(stats.Mean).c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(
                        mr.MetricObj->format(stats.Std).c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(
                        mr.MetricObj->format(stats.Min).c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(
                        mr.MetricObj->format(stats.Max).c_str());
                }

                ImGui::EndTable();
            }
        }

        // --- Single-value metrics (e.g. RMSE) ---
        if (has_single) {
            for (const auto &mr : res.Metrics) {
                if (mr.MetricObj->showDistribution())
                    continue;
                const std::string NAME(mr.MetricObj->name());
                ImGui::Text("%s: %s", _UI(NAME.c_str()),
                            mr.MetricObj->format(mr.Stats.Mean).c_str());
            }
        }

        ImGui::Separator();
    }

    // Last iteration info
    ImGui::Separator();
    ImGui::Text(_UI("Last iteration:"));
    if (!res.LastPeaks.empty()) {
        for (std::size_t i = 0; i < res.LastPeaks.size(); ++i) {
            ImGui::Text(_UI("  Peak %zu: %.3f Hz  (amp = %.4e)"), i,
                        res.LastPeaks[i].FrequencyHz,
                        res.LastPeaks[i].Amplitude);
        }
    } else {
        ImGui::Text(_UI("  (no peaks detected)"));
    }

    ImGui::End();
}

} // namespace ispp::ui
