#include "ispp/ui/panels/config_panel.h"
#include "ispp/core/parameters.h"
#include "ispp/estimator/esprit.h"
#include "ispp/estimator/fft_interpolate.h"
#include "ispp/estimator/fft_peak.h"
#include "ispp/estimator/music.h"
#include "ispp/metrics/compute_time.h"
#include "ispp/metrics/percentage_error.h"
#include "ispp/metrics/rmse.h"
#include "ispp/ui/widgets/enum_combo.h"
#include <array>
#include <imgui.h>

namespace ispp::ui {

static constexpr std::array WINDOW_NAMES = {"Rectangular", "Hamming", "Hann",
                                            "Blackman"};
static constexpr std::array NOISE_NAMES = {"Gaussian", "Uniform", "Laplacian",
                                           "Impulse"};
static constexpr std::array ALGO_NAMES = {"FFT Peak", "FFT Interpolate",
                                          "MUSIC", "ESPRIT"};
static constexpr std::array METRIC_NAMES = {"Percentage Error", "MSE",
                                            "Compute Time"};

void ConfigPanel::render(ExperimentConfig &config, RunState &state,
                         std::shared_ptr<IEstimator> &estimator,
                         std::vector<std::shared_ptr<IMetric>> &metrics) {
    if (!ImGui::Begin("Configuration")) {
        ImGui::End();
        return;
    }

    // ---- Signal parameters ----
    if (ImGui::CollapsingHeader("Signal", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Signal");
        ImGui::InputDouble("Sample Rate (Hz)", &config.Signal.SampleRateHz,
                           1000.0, 10000.0, "%.0f");
        ImGui::InputScalar("Sample Count", ImGuiDataType_U64,
                           &config.Signal.SampleCount);
        ImGui::InputDouble("Frequency (Hz)", &config.Signal.FrequencyHz, 1.0,
                           10.0, "%.1f");
        ImGui::InputDouble("Amplitude", &config.Signal.Amplitude, 0.1, 1.0,
                           "%.2f");
        ImGui::InputDouble("Phase (rad)", &config.Signal.PhaseRad, 0.1, 1.0,
                           "%.3f");
        ImGui::PopID();
    }

    // ---- Noise ----
    if (ImGui::CollapsingHeader("Noise", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Noise");
        enumCombo("Distribution", &config.Env.Noise.Distribution,
                  NOISE_NAMES.data(), static_cast<int>(NOISE_NAMES.size()));
        double snr_min = -20.0, snr_max = 30.0;
        ImGui::SliderScalar("SNR (dB)", ImGuiDataType_Double,
                            &config.Env.Noise.SnrDb, &snr_min, &snr_max,
                            "%.1f");
        ImGui::PopID();
    }

    // ---- Window ----
    if (ImGui::CollapsingHeader("Window", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Window");
        enumCombo("Kind", &config.Env.Window.Kind, WINDOW_NAMES.data(),
                  static_cast<int>(WINDOW_NAMES.size()));
        ImGui::PopID();
    }

    // ---- Interference ----
    if (ImGui::CollapsingHeader("Interference",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Interference");
        double delta_min = -2.0, delta_max = 2.0;
        ImGui::SliderScalar("Delta (bins)", ImGuiDataType_Double,
                            &config.Env.Interference.DeltaBins, &delta_min,
                            &delta_max, "%.2f");
        ImGui::InputDouble("Amplitude", &config.Env.Interference.Amplitude, 0.1,
                           1.0, "%.2f");
        ImGui::PopID();
    }

    // ---- Algorithm & config ----
    ImGui::SeparatorText("Estimator");
    ImGui::Combo("Algorithm", &SelectedAlgorithm, ALGO_NAMES.data(),
                 static_cast<int>(ALGO_NAMES.size()));
    ImGui::InputScalar("Max Frequency Count", ImGuiDataType_U64,
                       &config.MaxFreqCount);

    ImGui::SeparatorText("Monte Carlo");
    ImGui::InputScalar("Iterations", ImGuiDataType_U64,
                       &config.MonteCarlo.IterationCount);
    ImGui::InputScalar("Base Seed", ImGuiDataType_U64,
                       &config.MonteCarlo.BaseSeed);

    ImGui::SeparatorText("Metrics");
    for (int i = 0; i < 3; ++i)
        ImGui::Checkbox(METRIC_NAMES[static_cast<std::size_t>(i)],
                        &MetricsMask[static_cast<std::size_t>(i)]);

    // ---- Run button ----
    ImGui::Separator();
    if (state.Running) {
        ImGui::ProgressBar(state.Progress, ImVec2(-1, 0), "Running...");
    } else if (ImGui::Button("Run Experiment", ImVec2(-1, 36))) {
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
        if (MetricsMask[0])
            metrics.push_back(std::make_shared<PercentageErrorMetric>());
        if (MetricsMask[1])
            metrics.push_back(std::make_shared<RmseMetric>());
        if (MetricsMask[2])
            metrics.push_back(std::make_shared<ComputeTimeMetric>());
        state.Pending = true;
    }

    ImGui::End();
}

} // namespace ispp::ui
