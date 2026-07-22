#include "ispp/ui/panels/spectrum_panel.h"
#include "ispp/i18n.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <imgui.h>
#include <implot.h>
#include <minwindef.h>
#include <vector>

namespace ispp::ui {

void SpectrumPanel::render(const std::optional<RunResult> &result) {
    if (!ImGui::Begin(_UI("Single Simulation"))) {
        ImGui::End();
        return;
    }

    if (!result || result->LastInputSignal.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1),
                           _UI("No data — run an experiment first."));
        ImGui::End();
        return;
    }

    const auto &res = *result;
    const std::size_t N = res.LastInputSignal.size();
    const std::size_t OUT_SIZE = res.LastSpectrumFreqHz.size();

    // Detect new experiment data: the vector's heap pointer changes when
    // UiManager move-assigns a fresh RunResult. Force axis limits only on
    // that frame; thereafter ImPlotCond_Once is a no-op so user zoom is
    // preserved.
    const double *current_ptr = res.LastInputSignal.data();
    const ImPlotCond COND =
        (current_ptr != LastInputSignalData) ? ImPlotCond_Always : ImPlotCond_Once;
    LastInputSignalData = current_ptr;

    // ---- Time-domain signal ----
    const float AVAIL_X = ImGui::GetContentRegionAvail().x;
    ImGui::SetNextWindowSizeConstraints(ImVec2(AVAIL_X, 0),
                                        ImVec2(AVAIL_X, FLT_MAX));
    ImGui::BeginChild("WaveformContainer", ImVec2(0, 280),
                      ImGuiChildFlags_ResizeY);
    if (ImPlot::BeginPlot(_UI("Waveform"), ImVec2(-1, -1))) {
        ImPlot::SetupAxis(ImAxis_X1, _UI("Sample"));
        ImPlot::SetupAxis(ImAxis_Y1, _UI("Amplitude"));
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0,
                                static_cast<double>(N - 1), COND);
        const auto [MIN_SIG_IT, MAX_SIG_IT] =
            std::ranges::minmax_element(res.LastInputSignal);
        const auto DELTA_SIG = (*MAX_SIG_IT - *MIN_SIG_IT) * 0.08;
        ImPlot::SetupAxisLimits(ImAxis_Y1, *MIN_SIG_IT - DELTA_SIG,
                                *MAX_SIG_IT + DELTA_SIG, COND);
        ImPlot::PlotLine(_UI("Input"), res.LastInputSignal.data(),
                         static_cast<int>(N));
        ImPlot::EndPlot();
    }
    ImGui::EndChild();

    // ---- Frequency-domain spectrum (dB) ----
    std::vector<double> mag_db(OUT_SIZE);
    for (std::size_t k = 0; k < OUT_SIZE; ++k) {
        double m = res.LastSpectrumMag[k];
        mag_db[k] = (m > 1e-15) ? 20.0 * std::log10(m) : -300.0;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(AVAIL_X, 0),
                                        ImVec2(AVAIL_X, FLT_MAX));
    ImGui::BeginChild("SpectrumContainer", ImVec2(0, 280),
                      ImGuiChildFlags_ResizeY);
    if (ImPlot::BeginPlot(_UI("Spectrum"), ImVec2(-1, -1))) {
        ImPlot::SetupAxis(ImAxis_X1, _UI("Frequency (Hz)"));
        ImPlot::SetupAxis(ImAxis_Y1, _UI("Magnitude (dB)"));
        ImPlot::SetupAxisLimits(ImAxis_X1, res.LastSpectrumFreqHz.front(),
                                res.LastSpectrumFreqHz.back(), COND);
        const auto [MIN_MAG_IT, MAX_MAG_IT] =
            std::ranges::minmax_element(mag_db);
        const auto DELTA_MAG = (*MAX_MAG_IT - *MIN_MAG_IT) * 0.08;
        ImPlot::SetupAxisLimits(ImAxis_Y1, *MIN_MAG_IT - DELTA_MAG,
                                *MAX_MAG_IT + DELTA_MAG, COND);
        ImPlot::PlotLine(_UI("Spectrum"), res.LastSpectrumFreqHz.data(),
                         mag_db.data(), static_cast<int>(OUT_SIZE));
        // True frequency reference line (vertical)
        if (res.LastTrueFrequencyHz > 0) {
            const double TRUE_FREQ = res.LastTrueFrequencyHz;
            ImPlot::PlotInfLines(_UI("True Freq"), &TRUE_FREQ, 1);
            // Interference frequency reference line (vertical)
            if (res.LastInterferenceDeltaHz != 0) {
                const double INTERFERENCE_FREQ =
                    TRUE_FREQ + res.LastInterferenceDeltaHz;
                ImPlot::PlotInfLines(_UI("Intfc Freq"), &INTERFERENCE_FREQ, 1);
            }
        }
        // Estimated peaks as scatter
        if (!res.LastPeaks.empty()) {
            std::vector<double> peak_freqs(res.LastPeaks.size());
            std::vector<double> peak_dbs(res.LastPeaks.size());
            for (std::size_t i = 0; i < res.LastPeaks.size(); ++i) {
                peak_freqs[i] = res.LastPeaks[i].FrequencyHz;
                // NOLINTNEXTLINE(modernize-use-ranges)
                auto it = std::lower_bound(res.LastSpectrumFreqHz.begin(),
                                           res.LastSpectrumFreqHz.end(),
                                           peak_freqs[i]);
                std::size_t idx = static_cast<std::size_t>(
                    it - res.LastSpectrumFreqHz.begin());
                idx = std::min(idx, OUT_SIZE - 1);
                double m = res.LastSpectrumMag[idx];
                peak_dbs[i] = (m > 1e-15) ? 20.0 * std::log10(m) : -300.0;
            }
            ImPlot::PlotScatter(_UI("Peaks"), peak_freqs.data(),
                                peak_dbs.data(),
                                static_cast<int>(peak_freqs.size()));
        }
        ImPlot::EndPlot();
    }
    ImGui::EndChild();

    ImGui::End();
}

} // namespace ispp::ui
