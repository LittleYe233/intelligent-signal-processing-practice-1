#ifndef ISPP_EXPERIMENT_EXPERIMENT_RUNNER_H
#define ISPP_EXPERIMENT_EXPERIMENT_RUNNER_H

#include "ispp/estimator/estimator.h"
#include "ispp/experiment/experiment_config.h"
#include "ispp/experiment/statistics.h"
#include "ispp/metrics/metric.h"
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace ispp {

// ---------------------------------------------------------------------------
// 单指标运行结果
// ---------------------------------------------------------------------------
struct MetricResult {
    std::shared_ptr<IMetric> MetricObj;
    MetricStats Stats;
};

// ---------------------------------------------------------------------------
// 单次实验完整运行结果
// ---------------------------------------------------------------------------
struct RunResult {
    // 每个指标的结果（按注册顺序；MetricObj 提供 format() 与
    // showDistribution()）
    std::vector<MetricResult> Metrics;

    // 末次迭代的可视化数据（供 UI 频谱面板使用）
    RealArray LastInputSignal;
    RealArray LastSpectrumFreqHz; // 频谱横轴
    RealArray LastSpectrumMag;    // 频谱纵轴
    std::vector<FrequencyPeak> LastPeaks;
    double LastTrueFrequencyHz;
    double LastInterferenceDeltaHz; // 干扰频偏

    // 整体运行耗时（含蒙特卡洛全程；用于显示，非评价指标）
    double TotalRuntimeSec;
};

// ---------------------------------------------------------------------------
// 蒙特卡洛实验驱动器
// ---------------------------------------------------------------------------
class ExperimentRunner {
public:
    using ProgressCallback = std::function<void(float /* [0, 1] */)>;

    ExperimentRunner(ExperimentConfig config,
                     std::shared_ptr<IEstimator> estimator,
                     std::vector<std::shared_ptr<IMetric>> metrics);

    /// 同步运行；后台线程由 UI 调用方包装。
    /// on_progress 在每次蒙特卡洛迭代结束时回调（线程安全：仅 UI 端读取）。
    RunResult run(const ProgressCallback &on_progress = nullptr);

    /// 取消标志（UI 可在另一线程设置）。
    void cancel();
    bool isCancelled() const;

private:
    ExperimentConfig Config;
    std::shared_ptr<IEstimator> Estimator;
    std::vector<std::shared_ptr<IMetric>> Metrics;
    std::atomic<bool> Cancelled{false};
};

} // namespace ispp

#endif // ISPP_EXPERIMENT_EXPERIMENT_RUNNER_H
