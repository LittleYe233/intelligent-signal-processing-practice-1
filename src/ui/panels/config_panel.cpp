#include "ispp/ui/panels/config_panel.h"
#include "ispp/core/parameters.h"
#include "ispp/estimator/esprit.h"
#include "ispp/estimator/fft_interpolate.h"
#include "ispp/estimator/fft_peak.h"
#include "ispp/estimator/music.h"
#include "ispp/i18n.h"
#include "ispp/metrics/compute_time.h"
#include "ispp/metrics/percentage_error.h"
#include "ispp/metrics/rmse.h"
#include "ispp/ui/widgets/enum_combo.h"
#include <array>
#include <imgui.h>

namespace ispp::ui {

void ConfigPanel::render(ExperimentConfig &config, RunState &state,
                         std::shared_ptr<IEstimator> &estimator,
                         std::vector<std::shared_ptr<IMetric>> &metrics) {

    static std::array window_names = {_UI("Rectangular"), _UI("Hamming"),
                                      _UI("Hann"), _UI("Blackman")};
    static std::array noise_names = {_UI("Gaussian"), _UI("Uniform"),
                                     _UI("Laplacian"), _UI("Impulse")};
    static std::array algo_names = {_UI("FFT Peak"), _UI("FFT Interpolate"),
                                    _UI("MUSIC"), _UI("ESPRIT")};

    if (!ImGui::Begin(_UI("Configuration"))) {
        ImGui::End();
        return;
    }

    // ---- Signal parameters ----
    if (ImGui::CollapsingHeader(_UI("Signal"),
                                ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Signal");
        ImGui::InputDouble(_UI("Sample Rate (Hz)"), &config.Signal.SampleRateHz,
                           1000.0, 10000.0, "%.0f");
        ImGui::InputScalar(_UI("Sample Count"), ImGuiDataType_U64,
                           &config.Signal.SampleCount);
        ImGui::InputDouble(_UI("Frequency (Hz)"), &config.Signal.FrequencyHz,
                           1.0, 10.0, "%.1f");
        ImGui::InputDouble(_UI("Phase (rad)"), &config.Signal.PhaseRad, 0.1,
                           1.0, "%.3f");
        ImGui::PopID();
    }

    // ---- Noise ----
    if (ImGui::CollapsingHeader(_UI("Noise"), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Noise");
        enumCombo(_UI("Distribution"), &config.Env.Noise.Distribution,
                  noise_names.data(), static_cast<int>(noise_names.size()));
        static constexpr double SNR_MIN = -20.0, SNR_MAX = 30.0;
        ImGui::SliderScalar(_UI("SNR (dB)"), ImGuiDataType_Double,
                            &config.Env.Noise.SnrDb, &SNR_MIN, &SNR_MAX,
                            "%.1f");
        ImGui::PopID();
    }

    // ---- Window ----
    if (ImGui::CollapsingHeader(_UI("Window"),
                                ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Window");
        enumCombo(_UI("Kind"), &config.Env.Window.Kind, window_names.data(),
                  static_cast<int>(window_names.size()));
        ImGui::PopID();
    }

    // ---- Interference ----
    if (ImGui::CollapsingHeader(_UI("Interference"),
                                ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Interference");
        static constexpr double DELTA_MIN = -4.0, DELTA_MAX = 4.0;
        if (ImGui::SliderScalar(_UI("Delta (bins)"), ImGuiDataType_Double,
                                &config.Env.Interference.DeltaBins, &DELTA_MIN,
                                &DELTA_MAX, "%.1f")) {
            // If uses "%.2f", uncomment below to set fixed step
            // config.Env.Interference.DeltaBins =
            //     std::round(config.Env.Interference.DeltaBins / 0.05f) *
            //     0.05f;
        }
        ImGui::InputDouble(_UI("Amplitude"), &config.Env.Interference.Amplitude,
                           0.1, 1.0, "%.2f");
        ImGui::PopID();
    }

    // ---- Algorithm & config ----
    ImGui::SeparatorText(_UI("Estimator"));
    ImGui::Combo(_UI("Algorithm"), &SelectedAlgorithm, algo_names.data(),
                 static_cast<int>(algo_names.size()));
    ImGui::InputScalar(_UI("Max Frequency Count"), ImGuiDataType_U64,
                       &config.MaxFreqCount);

    ImGui::SeparatorText(_UI("Monte Carlo"));
    ImGui::InputScalar(_UI("Iterations"), ImGuiDataType_U64,
                       &config.MonteCarlo.IterationCount);
    ImGui::InputScalar(_UI("Base Seed"), ImGuiDataType_U64,
                       &config.MonteCarlo.BaseSeed);

    // ---- Run button ----
    ImGui::Separator();
    if (state.Running) {
        ImGui::ProgressBar(state.Progress, ImVec2(-1, 0), _UI("Running..."));
    } else if (ImGui::Button(_UI("Run Experiment"), ImVec2(-1, 36))) {
        const double THRESHOLD = 0.0;
        switch (SelectedAlgorithm) {
        case 0:
            estimator = std::make_shared<FftPeakEstimator>(THRESHOLD);
            break;
        case 1:
            estimator = std::make_shared<FftInterpolateEstimator>(THRESHOLD);
            break;
        case 2:
            estimator = std::make_shared<MusicEstimator>();
            break;
        case 3:
            estimator = std::make_shared<EspritEstimator>();
            break;
        default:
            break;
        }
        metrics.clear();
        metrics.push_back(std::make_shared<PercentageErrorMetric>());
        metrics.push_back(std::make_shared<RmseMetric>());
        metrics.push_back(std::make_shared<ComputeTimeMetric>());
        state.Pending = true;
    }

    ImGui::End();
}

} // namespace ispp::ui
