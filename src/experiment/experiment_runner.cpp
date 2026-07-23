#include "ispp/experiment/experiment_runner.h"
#include "ispp/core/fft.h"
#include "ispp/core/rng.h"
#include "ispp/signal/signal_generator.h"
#include "ispp/window/window.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

namespace ispp {

ExperimentRunner::ExperimentRunner(
    ExperimentConfig config, std::shared_ptr<IEstimator> estimator,
    std::vector<std::shared_ptr<IMetric>> metrics)
    : Config(config), Estimator(std::move(estimator)),
      Metrics(std::move(metrics)) {}

void ExperimentRunner::cancel() { Cancelled.store(true); }

bool ExperimentRunner::isCancelled() const { return Cancelled.load(); }

// ---------------------------------------------------------------------------
// run() — 蒙特卡洛主循环
// ---------------------------------------------------------------------------
RunResult ExperimentRunner::run(const ProgressCallback &on_progress) {
    RunResult result;
    result.LastTrueFrequencyHz = Config.Signal.FrequencyHz;
    result.LastInterferenceDeltaHz =
        Config.Env.Interference.DeltaBins * Config.Signal.SampleRateHz /
        static_cast<double>(Config.Signal.SampleCount);

    const double SAMPLE_RATE = Config.Signal.SampleRateHz;
    const std::size_t N = Config.Signal.SampleCount;
    const double BIN_HZ = SAMPLE_RATE / static_cast<double>(N);
    const std::size_t MC_CNT = Config.MonteCarlo.IterationCount;

    // 每个指标收集一次蒙特卡洛的样本值
    std::vector<std::vector<double>> metric_samples(Metrics.size());

    // 检查是否存在聚合指标（OQ-20）
    bool has_aggregate = false;
    for (const auto &m : Metrics) {
        if (m->isAggregate()) {
            has_aggregate = true;
            break;
        }
    }

    // 聚合指标需要的原始频率估计（OQ-21: max-Prominence 选峰）
    std::vector<double> freq_estimates;
    if (has_aggregate) {
        freq_estimates.reserve(MC_CNT);
    }

    // NoiseInfo + FrequencyCount：不随迭代变化，提前计算
    const NoiseInfo NOISE_INFO{.Distribution = Config.Env.Noise.Distribution,
                               .SnrDb = Config.Env.Noise.SnrDb};
    const std::size_t FREQ_CNT =
        Config.MaxFreqCount +
        (Config.Env.Interference.DeltaBins != 0.0 ? 1 : 0);

    auto overall_start = std::chrono::steady_clock::now();

    for (std::size_t iter = 0; iter < MC_CNT; ++iter) {
        // --- 取消检查 ---
        if (Cancelled.load()) {
            break;
        }

        // --- 1. 生成输入信号 ---
        Rng rng(Config.MonteCarlo.BaseSeed + iter);
        SignalGenerator generator;
        RealArray input = generator.generate(Config.Signal, Config.Env, rng);

        // --- 2. 窗函数（计时起点） ---
        auto compute_start = std::chrono::steady_clock::now();
        applyWindow(input, Config.Env.Window.Kind);

        // --- 3. 频率估计（由 Runner 组装 EstimationResult 并注入计时） ---
        const EstimationContext CTX{
            .SampleRateHz = SAMPLE_RATE,
            .WindowKind = Config.Env.Window.Kind,
            .FrequencyCount = FREQ_CNT,
            .NoiseInfo = NOISE_INFO,
        };
        auto peaks = Estimator->estimate(input, CTX);
        auto compute_end = std::chrono::steady_clock::now();

        double compute_sec =
            std::chrono::duration<double>(compute_end - compute_start).count();
        EstimationResult est_result{.Peaks = std::move(peaks),
                                    .ComputeTimeSec = compute_sec};

        // --- 4. 评价指标（跳过聚合指标） ---
        for (std::size_t m = 0; m < Metrics.size(); ++m) {
            if (Metrics[m]->isAggregate()) {
                continue;
            }
            double val =
                Metrics[m]->evaluate(Config.Signal.FrequencyHz, est_result);
            metric_samples[m].push_back(val);
        }

        // --- 4b. 收集原始频率估计（供聚合指标，OQ-21 max-Prominence） ---
        if (has_aggregate && !est_result.Peaks.empty()) {
            auto best = std::ranges::max_element(
                est_result.Peaks, std::less<>{},
                [](const FrequencyPeak &p) { return p.Prominence; });
            freq_estimates.push_back(best->FrequencyHz);
        }

        // --- 5. 末次迭代缓存（可视化数据） ---
        bool is_last = (iter == MC_CNT - 1) || Cancelled.load();
        if (is_last) {
            result.LastInputSignal = input;
            result.LastPeaks = est_result.Peaks;

            // 复用公共 FFT 工具计算单边幅度谱
            const ComplexArray SPECTRUM = computeDft(input);
            const std::size_t OUT_SIZE = SPECTRUM.size();
            result.LastSpectrumFreqHz.resize(OUT_SIZE);
            result.LastSpectrumMag.resize(OUT_SIZE);
            for (std::size_t k = 0; k < OUT_SIZE; ++k) {
                result.LastSpectrumFreqHz[k] = static_cast<double>(k) * BIN_HZ;
                result.LastSpectrumMag[k] = std::abs(SPECTRUM[k]);
            }
        }

        // --- 6. 进度回调 ---
        if (on_progress) {
            float progress =
                static_cast<float>(iter + 1) / static_cast<float>(MC_CNT);
            on_progress(progress);
        }
    }

    auto overall_end = std::chrono::steady_clock::now();
    result.TotalRuntimeSec =
        std::chrono::duration<double>(overall_end - overall_start).count();

    // --- 聚合每指标统计 ---
    for (std::size_t m = 0; m < Metrics.size(); ++m) {
        if (Metrics[m]->isAggregate()) {
            const double VAL = Metrics[m]->finalize(freq_estimates, SAMPLE_RATE,
                                                    N, NOISE_INFO);
            result.Metrics.push_back({
                .MetricObj = Metrics[m],
                .Stats =
                    MetricStats{
                        .Mean = VAL, .Std = 0.0, .Min = 0.0, .Max = 0.0},
            });
        } else {
            result.Metrics.push_back({
                .MetricObj = Metrics[m],
                .Stats = computeStats(metric_samples[m]),
            });
        }
    }

    return result;
}

} // namespace ispp
