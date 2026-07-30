#include "ispp/experiment/scan_test_runner.h"
#include "ispp/estimator/esprit.h"
#include "ispp/estimator/fft_interpolate.h"
#include "ispp/estimator/fft_peak.h"
#include "ispp/estimator/music.h"
#include "ispp/experiment/experiment_runner.h"
#include "ispp/metrics/compute_time.h"
#include "ispp/metrics/mse.h"
#include "ispp/metrics/percentage_error.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace ispp {

// ===========================================================================
// ScanParam helpers
// ===========================================================================

bool isScanParamDiscrete(ScanParam param) {
    switch (param) {
    case ScanParam::SAMPLE_COUNT:
    case ScanParam::MAX_FREQ_COUNT:
    case ScanParam::WINDOW_KIND:
    case ScanParam::NOISE_DISTRIBUTION:
    case ScanParam::ALGORITHM:
        return true;
    default:
        return false;
    }
}

void applyScanParam(ExperimentConfig &config, ScanParam param, double value) {
    switch (param) {
    case ScanParam::SNR_DB:
        config.Env.Noise.SnrDb = value;
        break;
    case ScanParam::FREQUENCY_HZ:
        config.Signal.FrequencyHz = value;
        break;
    case ScanParam::PHASE_RAD:
        config.Signal.PhaseRad = value;
        break;
    case ScanParam::INTERFERENCE_DELTA_BINS:
        config.Env.Interference.DeltaBins = value;
        break;
    case ScanParam::INTERFERENCE_AMPLITUDE:
        config.Env.Interference.Amplitude = value;
        break;
    case ScanParam::SAMPLE_RATE_HZ:
        config.Signal.SampleRateHz = value;
        break;
    case ScanParam::SAMPLE_COUNT:
        config.Signal.SampleCount =
            static_cast<std::size_t>(std::llround(value));
        break;
    case ScanParam::MAX_FREQ_COUNT:
        config.MaxFreqCount = static_cast<std::size_t>(std::llround(value));
        break;
    case ScanParam::WINDOW_KIND:
        config.Env.Window.Kind =
            static_cast<WindowKind>(static_cast<int>(std::llround(value)));
        break;
    case ScanParam::NOISE_DISTRIBUTION:
        config.Env.Noise.Distribution = static_cast<NoiseDistribution>(
            static_cast<int>(std::llround(value)));
        break;
    case ScanParam::ALGORITHM:
        // Algorithm 不在 ExperimentConfig 中；由 run() 内联处理
        break;
    }
}

// ===========================================================================
// Algorithm registry
// ===========================================================================

std::vector<AlgorithmEntry> getAllAlgorithms() {
    std::vector<AlgorithmEntry> algos;
    algos.reserve(4);
    algos.push_back({.Name = "FFT Peak",
                     .Estimator = std::make_shared<FftPeakEstimator>(0.0)});
    algos.push_back(
        {.Name = "FFT Interpolate",
         .Estimator = std::make_shared<FftInterpolateEstimator>(0.0)});
    algos.push_back(
        {.Name = "MUSIC", .Estimator = std::make_shared<MusicEstimator>()});
    algos.push_back(
        {.Name = "ESPRIT", .Estimator = std::make_shared<EspritEstimator>()});
    return algos;
}

// ===========================================================================
// ScanTestRunner
// ===========================================================================

ScanTestRunner::ScanTestRunner() : Tests(buildDefaultTests()) {}

void ScanTestRunner::cancel() { Cancelled.store(true); }

bool ScanTestRunner::isCancelled() const { return Cancelled.load(); }

// ---------------------------------------------------------------------------
// 工具：构建默认 ExperimentConfig（§7.5.1）
// ---------------------------------------------------------------------------
static ExperimentConfig makeDefaultConfig() {
    ExperimentConfig cfg{};
    cfg.Signal.SampleRateHz = 1000.0;
    cfg.Signal.SampleCount = 256;
    cfg.Signal.FrequencyHz = 200.0;
    cfg.Signal.PhaseRad = 0.0;
    cfg.Env.Window.Kind = WindowKind::RECTANGULAR;
    cfg.Env.Noise.Distribution = NoiseDistribution::GAUSSIAN;
    cfg.Env.Noise.SnrDb = 10.0;
    cfg.Env.Interference.DeltaBins = 0.0;
    cfg.Env.Interference.Amplitude = 0.5;
    cfg.MaxFreqCount = 1;
    cfg.MonteCarlo.IterationCount = 100;
    cfg.MonteCarlo.BaseSeed = 7792565;
    return cfg;
}

// ---------------------------------------------------------------------------
// 工具：生成类别标签
// ---------------------------------------------------------------------------
static std::string labelForWindow(WindowKind kind) {
    switch (kind) {
    case WindowKind::RECTANGULAR:
        return "Rectangular";
    case WindowKind::HAMMING:
        return "Hamming";
    case WindowKind::HANN:
        return "Hann";
    case WindowKind::BLACKMAN:
        return "Blackman";
    }
    return "?";
}

static std::string labelForNoiseDist(NoiseDistribution dist) {
    switch (dist) {
    case NoiseDistribution::GAUSSIAN:
        return "Gaussian";
    case NoiseDistribution::UNIFORM:
        return "Uniform";
    case NoiseDistribution::LAPLACIAN:
        return "Laplacian";
    case NoiseDistribution::IMPULSE:
        return "Impulse";
    }
    return "?";
}

static std::string labelForAlgorithm(std::size_t idx) {
    const auto ALGOS = getAllAlgorithms();
    if (idx < ALGOS.size()) {
        return ALGOS[idx].Name;
    }
    return "?";
}

// ---------------------------------------------------------------------------
// buildDefaultTests — 7 项具体测试（§7.7）
// ---------------------------------------------------------------------------
std::vector<ScanTestDef> ScanTestRunner::buildDefaultTests() {
    const auto ALGOS = getAllAlgorithms();
    std::vector<ScanTestDef> tests;

    // ---- Test 1: SampleCount scan (algo as series, MULTI_LINE) ----
    {
        ScanTestDef t;
        t.Name = "SampleCount scan";
        t.XDim = {.Param = ScanParam::SAMPLE_COUNT,
                  .Values = {32.0, 64.0, 128.0, 256.0, 512.0, 1024.0},
                  .Labels = {}};
        t.SeriesDim = ScanDimension{.Param = ScanParam::ALGORITHM,
                                    .Values = {0.0, 1.0, 2.0, 3.0},
                                    .Labels = {ALGOS[0].Name, ALGOS[1].Name,
                                               ALGOS[2].Name, ALGOS[3].Name}};
        t.MetricNames = {"Percentage Error", "Compute Time"};
        t.Style = ChartStyle::MULTI_LINE;
        t.FixedEstimator = nullptr;
        tests.push_back(std::move(t));
    }

    // ---- Test 2: Frequency scan (algo as series, MULTI_LINE) ----
    {
        ScanTestDef t;
        t.Name = "Frequency scan";
        std::vector<double> freq_vals;
        freq_vals.reserve(11);
        for (int fi = 0; fi < 11; ++fi) {
            freq_vals.push_back(1500.0 + static_cast<double>(fi) * 3.0);
        }
        t.XDim = {.Param = ScanParam::FREQUENCY_HZ,
                  .Values = std::move(freq_vals),
                  .Labels = {}};
        t.SeriesDim = ScanDimension{.Param = ScanParam::ALGORITHM,
                                    .Values = {0.0, 1.0, 2.0, 3.0},
                                    .Labels = {ALGOS[0].Name, ALGOS[1].Name,
                                               ALGOS[2].Name, ALGOS[3].Name}};
        t.MetricNames = {"Percentage Error"};
        t.Style = ChartStyle::MULTI_LINE;
        t.Overrides.emplace_back(ScanParam::SAMPLE_RATE_HZ, 7680.0);
        // 重命名键以避免歧义；applyScanParam 处理 SampleRateHz
        tests.push_back(std::move(t));
    }

    // ---- Test 3: NoiseDist x Algorithm ----
    {
        ScanTestDef t;
        t.Name = "NoiseDist x Algorithm";
        t.XDim = {.Param = ScanParam::ALGORITHM,
                  .Values = {0.0, 1.0, 2.0, 3.0},
                  .Labels = {ALGOS[0].Name, ALGOS[1].Name, ALGOS[2].Name,
                             ALGOS[3].Name}};
        t.SeriesDim = ScanDimension{
            .Param = ScanParam::NOISE_DISTRIBUTION,
            .Values = {0.0, 1.0, 2.0, 3.0},
            .Labels = {labelForNoiseDist(NoiseDistribution::GAUSSIAN),
                       labelForNoiseDist(NoiseDistribution::UNIFORM),
                       labelForNoiseDist(NoiseDistribution::LAPLACIAN),
                       labelForNoiseDist(NoiseDistribution::IMPULSE)}};
        t.MetricNames = {"Percentage Error"};
        t.Style = ChartStyle::GROUPED_BARS_WITH_ERROR;
        t.Overrides = {{ScanParam::SNR_DB, -8.0}};
        t.FixedEstimator = nullptr;
        tests.push_back(std::move(t));
    }

    // ---- Test 4: SNR scan (algo as series, MULTI_LINE) ----
    {
        ScanTestDef t;
        t.Name = "SNR scan";
        std::vector<double> snr_vals;
        snr_vals.reserve(21);
        for (int si = 0; si < 21; ++si) {
            snr_vals.push_back(-30.0 + static_cast<double>(si) * 2.5);
        }
        t.XDim = {.Param = ScanParam::SNR_DB,
                  .Values = std::move(snr_vals),
                  .Labels = {}};
        t.SeriesDim = ScanDimension{.Param = ScanParam::ALGORITHM,
                                    .Values = {0.0, 1.0, 2.0, 3.0},
                                    .Labels = {ALGOS[0].Name, ALGOS[1].Name,
                                               ALGOS[2].Name, ALGOS[3].Name}};
        t.MetricNames = {"Percentage Error"};
        t.Style = ChartStyle::MULTI_LINE;
        t.FixedEstimator = nullptr;
        tests.push_back(std::move(t));
    }

    // ---- Test 5: SNR x SampleCount (per-algorithm: Interpolate/MUSIC/ESPRIT)
    // ----
    {
        ScanTestDef t;
        t.Name = "SNR x SampleCount";
        std::vector<double> snr_vals;
        snr_vals.reserve(21);
        for (int si = 0; si < 21; ++si) {
            snr_vals.push_back(-30.0 + static_cast<double>(si) * 2.5);
        }
        t.XDim = {.Param = ScanParam::SNR_DB,
                  .Values = std::move(snr_vals),
                  .Labels = {}};
        t.ChartDim = ScanDimension{
            .Param = ScanParam::ALGORITHM,
            .Values = {1.0, 2.0, 3.0},
            .Labels = {ALGOS[1].Name, ALGOS[2].Name, ALGOS[3].Name}};
        t.SeriesDim = {.Param = ScanParam::SAMPLE_COUNT,
                       .Values = {64.0, 128.0, 256.0, 512.0},
                       .Labels = {}};
        t.MetricNames = {"Percentage Error"};
        t.Style = ChartStyle::MULTI_LINE;
        t.FixedEstimator = nullptr;
        tests.push_back(std::move(t));
    }

    // ---- Test 6: Window x Algorithm (per-SNR: -3 dB and 10 dB) ----
    {
        ScanTestDef t;
        t.Name = "Window x Algorithm";
        t.XDim = {
            .Param = ScanParam::ALGORITHM,
            .Values = {1.0, 2.0, 3.0},
            .Labels = {labelForAlgorithm(1), labelForAlgorithm(2),
                       labelForAlgorithm(3)},
        };
        t.SeriesDim =
            ScanDimension{.Param = ScanParam::WINDOW_KIND,
                          .Values = {0.0, 1.0, 2.0, 3.0},
                          .Labels = {labelForWindow(WindowKind::RECTANGULAR),
                                     labelForWindow(WindowKind::HAMMING),
                                     labelForWindow(WindowKind::HANN),
                                     labelForWindow(WindowKind::BLACKMAN)}};
        t.ChartDim = ScanDimension{.Param = ScanParam::SNR_DB,
                                   .Values = {-3.0, 10.0},
                                   .Labels = {"-3 dB", "10 dB"}};
        t.MetricNames = {"Percentage Error"};
        t.Style = ChartStyle::GROUPED_BARS_WITH_ERROR;
        t.FixedEstimator = nullptr;
        tests.push_back(std::move(t));
    }

    // ---- Test 7: Interference scan ----
    {
        ScanTestDef t;
        t.Name = "Interference scan";
        std::vector<double> delta_vals;
        delta_vals.reserve(21);
        for (int di = 0; di < 21; ++di) {
            delta_vals.push_back(static_cast<double>(di) * 0.2);
        }
        t.XDim = {.Param = ScanParam::INTERFERENCE_DELTA_BINS,
                  .Values = std::move(delta_vals),
                  .Labels = {}};
        t.ChartDim = ScanDimension{.Param = ScanParam::ALGORITHM,
                                   .Values = {0.0, 1.0, 2.0, 3.0},
                                   .Labels = {ALGOS[0].Name, ALGOS[1].Name,
                                              ALGOS[2].Name, ALGOS[3].Name}};
        t.MetricNames = {};
        t.Style = ChartStyle::MULTI_LINE;
        t.PerPeak = true;
        t.FixedEstimator = nullptr;
        tests.push_back(std::move(t));
    }

    return tests;
}

// ---------------------------------------------------------------------------
// buildAllMetrics
// ---------------------------------------------------------------------------
std::vector<std::shared_ptr<IMetric>> ScanTestRunner::buildAllMetrics() {
    std::vector<std::shared_ptr<IMetric>> metrics;
    metrics.reserve(3);
    metrics.push_back(std::make_shared<PercentageErrorMetric>());
    metrics.push_back(std::make_shared<MseMetric>());
    metrics.push_back(std::make_shared<ComputeTimeMetric>());
    return metrics;
}

// ===========================================================================
// run() — 多维枚举流水线
// ===========================================================================
std::vector<ScanTestOutput>
ScanTestRunner::run(const ProgressCallback &on_progress) {
    std::vector<ScanTestOutput> outputs;
    Cancelled.store(false);

    // 估算总点数（用于进度计算）
    std::size_t total_points = 0;
    for (const auto &test : Tests) {
        std::size_t chart_n = test.ChartDim ? test.ChartDim->Values.size() : 1;
        std::size_t series_n =
            test.SeriesDim ? test.SeriesDim->Values.size() : 1;
        std::size_t n_metrics =
            test.MetricNames.empty() ? 1 : test.MetricNames.size();
        total_points +=
            chart_n * n_metrics * series_n * test.XDim.Values.size();
    }

    std::size_t completed = 0;

    for (const auto &test : Tests) {
        // ================================================================
        //  Per-peak mode: 逐峰百分比误差（test.PerPeak == true）
        // ================================================================
        if (test.PerPeak) {
            const std::vector<double> CHART_VALS =
                test.ChartDim ? test.ChartDim->Values
                              : std::vector<double>{0.0};
            const auto ALGOS = getAllAlgorithms();
            const std::size_t N_ALGO = ALGOS.size();
            const std::size_t N = test.XDim.Values.size();

            ScanTestOutput output;
            output.Name = test.Name;

            for (std::size_t ci = 0; ci < CHART_VALS.size(); ++ci) {
                const double CV = CHART_VALS[ci];

                // Collect per-peak errors for each X point
                // ranked_errors[xi] = sorted vector of |freq - true|/true*100%
                std::vector<std::vector<double>> ranked_errors(N);

                for (std::size_t xi = 0; xi < N; ++xi) {
                    if (Cancelled.load()) {
                        goto done;
                    }

                    const double XV = test.XDim.Values[xi];

                    ExperimentConfig cfg = makeDefaultConfig();
                    // Per-peak mode: single deterministic run
                    cfg.MonteCarlo.IterationCount = 1;
                    for (const auto &[sp, val] : test.Overrides) {
                        applyScanParam(cfg, sp, val);
                    }
                    applyScanParam(cfg, test.XDim.Param, XV);
                    if (test.ChartDim) {
                        applyScanParam(cfg, test.ChartDim->Param, CV);
                    }

                    // Resolve estimator
                    std::shared_ptr<IEstimator> est;
                    if (test.ChartDim &&
                        test.ChartDim->Param == ScanParam::ALGORITHM) {
                        const auto IDX =
                            static_cast<std::size_t>(std::llround(CV));
                        est = (IDX < N_ALGO) ? ALGOS[IDX].Estimator : nullptr;
                    } else {
                        est = test.FixedEstimator;
                    }
                    if (!est) {
                        est = ALGOS[0].Estimator;
                    }

                    auto metrics = buildAllMetrics();
                    ExperimentRunner runner(cfg, std::move(est),
                                            std::move(metrics));
                    RunResult result;
                    bool ok = true;
                    try {
                        result = runner.run();
                    } catch (...) {
                        ok = false;
                    }

                    // Extract peaks, compute percentage error per peak,
                    // sort ascending.
                    if (ok) {
                        const double TRUE_FREQ = cfg.Signal.FrequencyHz;
                        for (const auto &p : result.LastPeaks) {
                            const double ERR =
                                std::abs(p.FrequencyHz - TRUE_FREQ) /
                                TRUE_FREQ * 100.0;
                            ranked_errors[xi].push_back(ERR);
                        }
                        std::ranges::sort(ranked_errors[xi]);
                    }

                    ++completed;
                    if (on_progress) {
                        float p = static_cast<float>(completed) /
                                  static_cast<float>(total_points);
                        std::string log_msg = test.Name + " [pt " +
                                              std::to_string(xi + 1) + "/" +
                                              std::to_string(N) + "]";
                        on_progress(p, log_msg);
                    }
                }

                // Build chart with one series per peak rank
                ChartResult chart;
                chart.Style = ChartStyle::MULTI_LINE;
                chart.TestName = test.Name;
                chart.YLabel = "Percentage Error";
                chart.XValues = test.XDim.Values;
                chart.IsDiscrete = isScanParamDiscrete(test.XDim.Param);
                if (chart.IsDiscrete) {
                    chart.XLabel = "Category";
                } else {
                    chart.XLabel = "Delta [bins]";
                }

                // ChartDim 标签（原子 msgid，供 UI 线程本地化）
                if (test.ChartDim) {
                    if (ci < test.ChartDim->Labels.size()) {
                        chart.ChartDimLabel = test.ChartDim->Labels[ci];
                    } else {
                        chart.ChartDimLabel = std::to_string(CV);
                    }
                }

                // 英文组合标题（仅作 ImGui/ImPlot 稳定唯一 ID；
                // 显示标题由 UI 线程从 TestName/YLabel/ChartDimLabel
                // 本地化组合）
                std::string title = test.Name + " — Percentage Error";
                if (!chart.ChartDimLabel.empty()) {
                    title += " [" + chart.ChartDimLabel + "]";
                }
                chart.Title = title;

                // Find max rank across all X points
                std::size_t max_rank = 0;
                for (const auto &errs : ranked_errors) {
                    max_rank = std::max(max_rank, errs.size());
                }

                for (std::size_t rank = 0; rank < max_rank; ++rank) {
                    SeriesResult sr;
                    sr.Name = "Peak " + std::to_string(rank + 1);
                    // 标记逐峰排位，供 UI 线程用翻译后的 "Peak %d" 本地化。
                    sr.PeakRank = static_cast<int>(rank + 1);
                    sr.Means.reserve(N);
                    for (std::size_t xi = 0; xi < N; ++xi) {
                        if (rank < ranked_errors[xi].size()) {
                            sr.Means.push_back(ranked_errors[xi][rank]);
                        } else {
                            sr.Means.push_back(
                                std::numeric_limits<double>::quiet_NaN());
                        }
                    }
                    chart.Series.push_back(std::move(sr));
                }

                output.Charts.push_back(std::move(chart));
            }

            outputs.push_back(std::move(output));
            continue; // skip normal metric-charts pipeline
        }

        // ================================================================
        // 正常 pipeline（MetricNames → ChartResult）
        // ================================================================
        const std::vector<double> CHART_VALS =
            test.ChartDim ? test.ChartDim->Values : std::vector<double>{0.0};
        const std::vector<double> SERIES_VALS =
            test.SeriesDim ? test.SeriesDim->Values : std::vector<double>{0.0};

        ScanTestOutput output;
        output.Name = test.Name;

        // 预生成算法解析表（减少重复构造）
        const auto ALGOS = getAllAlgorithms();
        const std::size_t N_ALGO = ALGOS.size();

        // 每张分图 或 每个 metric → 一张图表
        const std::size_t N_METRICS = test.MetricNames.size();

        // Overview 缓冲：对每个 metric，收集 ChartDim 各值的均值线
        std::vector<std::vector<SeriesResult>> overview_data(N_METRICS);

        for (std::size_t ci = 0; ci < CHART_VALS.size(); ++ci) {
            const double CV = CHART_VALS[ci];

            // ChartDim 标签（原子 msgid，本 ci 内所有 metric 图表共享）
            std::string chart_dim_label;
            if (test.ChartDim) {
                if (ci < test.ChartDim->Labels.size()) {
                    chart_dim_label = test.ChartDim->Labels[ci];
                } else {
                    chart_dim_label = std::to_string(CV);
                }
            }

            // Phase 1: 一次性创建所有 ChartResult（每个 metric 一张图）
            std::vector<ChartResult> metric_charts;
            metric_charts.reserve(N_METRICS);

            for (const auto &metric_name : test.MetricNames) {
                ChartResult chart;
                chart.Style = test.Style;
                chart.YLabel = metric_name;

                chart.XValues = test.XDim.Values;
                chart.IsDiscrete = isScanParamDiscrete(test.XDim.Param);
                if (chart.IsDiscrete) {
                    if (!test.XDim.Labels.empty()) {
                        chart.XLabels = test.XDim.Labels;
                    } else {
                        chart.XLabels.reserve(chart.XValues.size());
                        for (const auto &v : chart.XValues) {
                            if (v == std::floor(v)) {
                                chart.XLabels.push_back(
                                    std::to_string(static_cast<int>(v)));
                            } else {
                                chart.XLabels.push_back(std::to_string(v));
                            }
                        }
                    }
                }

                // 标题（TestName/YLabel/ChartDimLabel 为原子 msgid；
                // 英文 Title 仅作稳定 ID，显示标题由 UI 本地化组合）
                chart.TestName = test.Name;
                chart.ChartDimLabel = chart_dim_label;
                std::string title = test.Name + " — " + metric_name;
                if (!chart_dim_label.empty()) {
                    title += " [" + chart_dim_label + "]";
                }
                chart.Title = title;

                // X 轴标签
                if (chart.IsDiscrete) {
                    chart.XLabel = "Category";
                } else {
                    switch (test.XDim.Param) {
                    case ScanParam::SNR_DB:
                        chart.XLabel = "SNR [dB]";
                        break;
                    case ScanParam::FREQUENCY_HZ:
                        chart.XLabel = "Frequency [Hz]";
                        break;
                    case ScanParam::INTERFERENCE_DELTA_BINS:
                        chart.XLabel = "Delta [bins]";
                        break;
                    default:
                        chart.XLabel = "Value";
                        break;
                    }
                }

                metric_charts.push_back(std::move(chart));
            }

            // Phase 2: 系列循环
            const std::size_t N = test.XDim.Values.size();

            for (std::size_t si = 0; si < SERIES_VALS.size(); ++si) {
                const double SV = SERIES_VALS[si];

                // 系列名称（一次计算，应用到所有 metric 图表）
                std::string series_name;
                if (test.SeriesDim) {
                    if (si < test.SeriesDim->Labels.size()) {
                        series_name = test.SeriesDim->Labels[si];
                    } else if (test.SeriesDim->Param ==
                               ScanParam::SAMPLE_COUNT) {
                        series_name =
                            "N=" + std::to_string(static_cast<int>(SV));
                    } else {
                        series_name = std::to_string(SV);
                    }
                } else {
                    series_name = "All";
                }

                // 为每个 metric 图表添加空 SeriesResult
                for (auto &mc : metric_charts) {
                    SeriesResult series;
                    series.Name = series_name;
                    series.Means.reserve(N);
                    series.Stds.reserve(N);
                    series.Mins.reserve(N);
                    series.Maxs.reserve(N);
                    mc.Series.push_back(std::move(series));
                }

                // Phase 3: X 轴点循环——一次实验，提取所有 metric
                for (std::size_t xi = 0; xi < N; ++xi) {
                    const double XV = test.XDim.Values[xi];

                    if (Cancelled.load()) {
                        goto done;
                    }

                    // 1. 构建配置（一次）
                    ExperimentConfig cfg = makeDefaultConfig();

                    for (const auto &[sp, val] : test.Overrides) {
                        applyScanParam(cfg, sp, val);
                    }

                    applyScanParam(cfg, test.XDim.Param, XV);
                    if (test.SeriesDim) {
                        applyScanParam(cfg, test.SeriesDim->Param, SV);
                    }
                    if (test.ChartDim) {
                        applyScanParam(cfg, test.ChartDim->Param, CV);
                    }

                    // 2. 解析估计器（一次）
                    std::shared_ptr<IEstimator> est;
                    if (test.ChartDim &&
                        test.ChartDim->Param == ScanParam::ALGORITHM) {
                        const auto IDX =
                            static_cast<std::size_t>(std::llround(CV));
                        est = (IDX < N_ALGO) ? ALGOS[IDX].Estimator : nullptr;
                    } else if (test.SeriesDim &&
                               test.SeriesDim->Param == ScanParam::ALGORITHM) {
                        const auto IDX =
                            static_cast<std::size_t>(std::llround(SV));
                        est = (IDX < N_ALGO) ? ALGOS[IDX].Estimator : nullptr;
                    } else if (test.XDim.Param == ScanParam::ALGORITHM) {
                        const auto IDX =
                            static_cast<std::size_t>(std::llround(XV));
                        est = (IDX < N_ALGO) ? ALGOS[IDX].Estimator : nullptr;
                    } else {
                        est = test.FixedEstimator;
                    }

                    if (!est) {
                        est = ALGOS[0].Estimator;
                    }

                    // 3. 运行一次实验
                    auto metrics = buildAllMetrics();
                    ExperimentRunner runner(cfg, std::move(est),
                                            std::move(metrics));
                    RunResult result;
                    bool experiment_ok = true;
                    try {
                        result = runner.run();
                    } catch (const std::exception &e) {
                        experiment_ok = false;
                        if (on_progress) {
                            on_progress(static_cast<float>(completed) /
                                            static_cast<float>(total_points),
                                        std::string("ERROR: ") + e.what());
                        }
                    } catch (...) {
                        experiment_ok = false;
                        if (on_progress) {
                            on_progress(static_cast<float>(completed) /
                                            static_cast<float>(total_points),
                                        "ERROR: Unknown exception.");
                        }
                    }

                    if (!experiment_ok) {
                        for (auto &mc : metric_charts) {
                            mc.Series[si].Means.push_back(0.0);
                        }
                        completed += N_METRICS;
                        continue;
                    }

                    // 4. 从单次结果提取所有 metric
                    for (std::size_t mi = 0; mi < N_METRICS; ++mi) {
                        const auto &metric_name = test.MetricNames[mi];
                        auto &series = metric_charts[mi].Series[si];

                        bool found = false;
                        for (const auto &mr : result.Metrics) {
                            // metric::name() 现返回英语 msgid（locale
                            // 无关的身份键）， 故直接与英语 metric_name
                            // 比较——worker 线程零 gettext。
                            if (mr.MetricObj->name() == metric_name) {
                                series.Means.push_back(mr.Stats.Mean);
                                if (mr.MetricObj->showDistribution()) {
                                    series.Stds.push_back(mr.Stats.Std);
                                    series.Mins.push_back(mr.Stats.Min);
                                    series.Maxs.push_back(mr.Stats.Max);
                                }
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            if (on_progress) {
                                on_progress(
                                    static_cast<float>(completed) /
                                        static_cast<float>(total_points),
                                    "WARNING: Metric '" + metric_name +
                                        "' not found.");
                            }
                            series.Means.push_back(0.0);
                        }
                    }

                    // 5. 进度
                    completed += N_METRICS;
                    if (on_progress) {
                        float p = static_cast<float>(completed) /
                                  static_cast<float>(total_points);
                        std::string log_msg = test.Name + " [pt " +
                                              std::to_string(xi + 1) + "/" +
                                              std::to_string(N) + "]";
                        on_progress(p, log_msg);
                    }
                }
            }

            // 若需总览图，记录当前 ChartDim 值的均值数据
            if (test.GenerateOverview && test.ChartDim) {
                std::string chart_label;
                if (ci < test.ChartDim->Labels.size()) {
                    chart_label = test.ChartDim->Labels[ci];
                } else {
                    chart_label = std::to_string(CV);
                }
                for (std::size_t mi = 0; mi < N_METRICS; ++mi) {
                    if (!metric_charts[mi].Series.empty()) {
                        SeriesResult sr;
                        sr.Name = chart_label;
                        sr.Means = metric_charts[mi].Series[0].Means;
                        overview_data[mi].push_back(std::move(sr));
                    }
                }
            }

            // 将所有 metric 图表移至输出
            for (auto &mc : metric_charts) {
                output.Charts.push_back(std::move(mc));
            }
        }

        // 生成总览图（各 ChartDim 均值的 MULTI_LINE 叠图）
        if (test.GenerateOverview) {
            for (std::size_t mi = 0; mi < N_METRICS; ++mi) {
                if (overview_data[mi].empty())
                    continue;

                ChartResult combined;
                combined.Style = ChartStyle::MULTI_LINE;
                combined.TestName = test.Name;
                combined.IsOverview = true;
                combined.YLabel = test.MetricNames[mi];
                combined.Title =
                    test.Name + " — " + test.MetricNames[mi] + " (overview)";
                combined.XValues = test.XDim.Values;
                combined.IsDiscrete = isScanParamDiscrete(test.XDim.Param);
                if (combined.IsDiscrete && !test.XDim.Labels.empty()) {
                    combined.XLabels = test.XDim.Labels;
                }
                if (combined.IsDiscrete) {
                    combined.XLabel = "Category";
                } else {
                    switch (test.XDim.Param) {
                    case ScanParam::SNR_DB:
                        combined.XLabel = "SNR [dB]";
                        break;
                    case ScanParam::FREQUENCY_HZ:
                        combined.XLabel = "Frequency [Hz]";
                        break;
                    case ScanParam::INTERFERENCE_DELTA_BINS:
                        combined.XLabel = "Delta [bins]";
                        break;
                    default:
                        combined.XLabel = "Value";
                        break;
                    }
                }
                combined.Series = std::move(overview_data[mi]);
                output.Charts.push_back(std::move(combined));
            }
        }

        outputs.push_back(std::move(output));
    }

done:
    return outputs;
}

} // namespace ispp
