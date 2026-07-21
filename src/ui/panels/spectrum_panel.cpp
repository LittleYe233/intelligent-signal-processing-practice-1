#include "ispp/ui/panels/spectrum_panel.h"

#include <imgui.h>
#include <implot.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace ispp::ui {

void SpectrumPanel::render(const std::optional<RunResult> &result) {
    if (!ImGui::Begin("Spectrum")) {
        ImGui::End();
        return;
    }

    if (!result || result->LastInputSignal.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1),
                           "No data — run an experiment first.");
        ImGui::End();
        return;
    }

    const auto &res = *result;
    const std::size_t N = res.LastInputSignal.size();
    const std::size_t OUT_SIZE = res.LastSpectrumFreqHz.size();

    // ---- Time-domain signal ----
    if (ImPlot::BeginPlot("Time Domain", ImVec2(-1, 200))) {
        ImPlot::SetupAxis(ImAxis_X1, "Sample");
        ImPlot::SetupAxis(ImAxis_Y1, "Amplitude");
        ImPlot::PlotLine("Input", res.LastInputSignal.data(),
                         static_cast<int>(N));
        ImPlot::EndPlot();
    }

    // ---- Frequency-domain spectrum (dB) ----
    std::vector<double> mag_db(OUT_SIZE);
    for (std::size_t k = 0; k < OUT_SIZE; ++k) {
        double m = res.LastSpectrumMag[k];
        mag_db[k] = (m > 1e-15) ? 20.0 * std::log10(m) : -300.0;
    }

    if (ImPlot::BeginPlot("Spectrum (dB)", ImVec2(-1, 280))) {
        ImPlot::SetupAxis(ImAxis_X1, "Frequency (Hz)");
        ImPlot::SetupAxis(ImAxis_Y1, "Magnitude (dB)");

        ImPlot::PlotLine("Spectrum", res.LastSpectrumFreqHz.data(),
                         mag_db.data(), static_cast<int>(OUT_SIZE));

        // True frequency reference line (vertical)
        if (res.LastTrueFrequencyHz > 0) {
            double true_freq = res.LastTrueFrequencyHz;
            ImPlot::PlotInfLines("True Freq", &true_freq, 1);
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
            ImPlot::PlotScatter("Peaks", peak_freqs.data(), peak_dbs.data(),
                                static_cast<int>(peak_freqs.size()));
        }

        ImPlot::EndPlot();
    }

    ImGui::End();
}

} // namespace ispp::ui
