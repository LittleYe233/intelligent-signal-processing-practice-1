#ifndef ISPP_EXPERIMENT_SCAN_TEST_RUNNER_H
#define ISPP_EXPERIMENT_SCAN_TEST_RUNNER_H

#include "ispp/estimator/estimator.h"
#include "ispp/experiment/experiment_config.h"
#include "ispp/metrics/metric.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace ispp {

// ---------------------------------------------------------------------------
// 7.5.2 可扫描参数枚举
// ---------------------------------------------------------------------------
enum class ScanParam : std::uint8_t {
    // 连续型
    SNR_DB,
    FREQUENCY_HZ,
    PHASE_RAD,
    INTERFERENCE_DELTA_BINS,
    INTERFERENCE_AMPLITUDE,
    SAMPLE_RATE_HZ,
    // 离散型（数值）
    SAMPLE_COUNT,
    MAX_FREQ_COUNT,
    // 离散型（类别）
    WINDOW_KIND,
    NOISE_DISTRIBUTION,
    // 特殊：不在 ExperimentConfig 中，需解析为 IEstimator
    ALGORITHM,
};

bool isScanParamDiscrete(ScanParam param);

void applyScanParam(ExperimentConfig &config, ScanParam param, double value);

// ---------------------------------------------------------------------------
// 7.5.2 算法注册表
// ---------------------------------------------------------------------------
struct AlgorithmEntry {
    std::string Name;
    std::shared_ptr<IEstimator> Estimator;
};

extern const std::vector<AlgorithmEntry> ALL_ALGORITHMS;

// ---------------------------------------------------------------------------
// 7.5.3 图表样式枚举
// ---------------------------------------------------------------------------
enum class ChartStyle : std::uint8_t {
    LINE_WITH_ERROR_BANDS,   // 折线 + 误差带（mean ± std, min~max bands）
    GROUPED_BARS_WITH_ERROR, // 分组柱状图 + 误差须
    MULTI_LINE,              // 多折线（仅均值，不同线型）
};

// ---------------------------------------------------------------------------
// 7.5.4 扫描维度
// ---------------------------------------------------------------------------
struct ScanDimension {
    ScanParam Param;
    std::vector<double> Values;
    std::vector<std::string> Labels; // 离散参数的类别标签（平行于 Values）
};

// ---------------------------------------------------------------------------
// 7.5.4 Custom evaluator — replaces the standard "run + extract metric" step
// ---------------------------------------------------------------------------

/// Custom evaluator that replaces the standard ExperimentRunner →
/// metric-extraction pipeline for a single scan point.
///
/// Receives the fully-built ExperimentConfig (may be modified), the
/// resolved IEstimator, and the list of metric names to produce values for.
/// Returns exactly MetricNames.size() values in the same order.
///
/// The standard chart-building loop handles everything else (chart
/// creation, title assembly, progress reporting).
///
/// Usage: set ScanTestDef::CustomEval and populate MetricNames;
/// a 100-line special-case scope collapses to a ~15-line lambda.
using CustomEvaluator = std::function<std::vector<double>(
    ExperimentConfig config, const std::shared_ptr<IEstimator> &estimator,
    const std::vector<std::string> &metric_names)>;

// ---------------------------------------------------------------------------
// 7.5.4 测试规格
// ---------------------------------------------------------------------------
struct ScanTestDef {
    std::string Name;

    ScanDimension XDim;                     // 必选：X 轴变量
    std::optional<ScanDimension> SeriesDim; // 可选：同图系列变量
    std::optional<ScanDimension> ChartDim;  // 可选：分图变量

    std::vector<std::string> MetricNames; // 每个 metric → 一组分图
    ChartStyle Style;

    // 配置覆盖（区别于全局默认的常量值）
    std::vector<std::pair<ScanParam, double>> Overrides;

    // 固定估计器（当 Algorithm 不是 ChartDim/SeriesDim 时使用）
    std::shared_ptr<IEstimator> FixedEstimator;

    // 若为 true，将各 ChartDim 值的均值合并为一张 MULTI_LINE 总览图
    bool GenerateOverview = false;

    // 若为 true，逐峰提取百分比误差（而非聚合到单一 metric）。
    // 每个 X 点运行 MC=1（确定性），将 LastPeaks 按距真频误差排序，
    // 每个排位画一条折线（MULTI_LINE）。
    bool PerPeak = false;

    // 若已设置，替代标准 ExperimentRunner + metric 提取流水线。
    // 典型用途：ComputeTimeRatio（需两次实验取比值）等自定义逻辑。
    CustomEvaluator CustomEval;
};

// ---------------------------------------------------------------------------
// 7.5.4 测试结果
// ---------------------------------------------------------------------------
struct SeriesResult {
    std::string Name; // 图例标签
    std::vector<double> Means;
    std::vector<double> Stds; // 空 = metric 无分布统计
    std::vector<double> Mins;
    std::vector<double> Maxs;

    // 逐峰模式（test 7）的峰排位（1-based）；< 0 表示非逐峰系列。
    // UI 线程据此用翻译后的 "Peak %d" 格式生成本地化图例（worker 线程
    // 不调用 _UI()，故无法在生成期本地化）。
    int PeakRank = -1;
};

struct ChartResult {
    std::string Title;
    std::string XLabel;
    std::string YLabel;
    ChartStyle Style;
    std::vector<double> XValues;
    std::vector<std::string> XLabels; // 离散时使用
    bool IsDiscrete = false;
    std::vector<SeriesResult> Series;

    // i18n 原子标题分量（OQ-i18n）：worker 线程存英语 msgid 字面量，
    // UI 线程据此组合本地化标题。详见 ScanResultsPanel::localizedTitle()。
    // Title 仍保留英文组合串，用作 ImGui/ImPlot 稳定唯一 ID。
    std::string TestName; // 测试名 msgid（如 "SNR scan"）
    std::string
        ChartDimLabel; // 分图维度标签 msgid（如 "FFT Peak"）；无分图时为空
    bool IsOverview = false; // 是否为总览图（组合各 ChartDim 均值）
};

struct ScanTestOutput {
    std::string Name;
    std::vector<ChartResult> Charts;
};

// ---------------------------------------------------------------------------
// 7.6 批量扫描测试驱动器
// ---------------------------------------------------------------------------
class ScanTestRunner {
public:
    using ProgressCallback =
        std::function<void(float progress, const std::string &test_name)>;

    ScanTestRunner();

    /// 同步运行所有扫描测试；后台线程由 UI 调用方包装。
    /// on_progress 在每次扫描点完成时回调（线程安全：仅 UI 端读取）。
    std::vector<ScanTestOutput>
    run(const ProgressCallback &on_progress = nullptr);

    /// 取消标志（UI 可在另一线程设置）。
    void cancel();
    bool isCancelled() const;

private:
    std::vector<ScanTestDef> Tests;

    /// 硬编码的扫描测试规格列表（7 项测试，§7.7）。
    static std::vector<ScanTestDef> buildDefaultTests();

    /// 构建全量活跃指标列表（当前为 PercentageError + MSE + ComputeTime）。
    static std::vector<std::shared_ptr<IMetric>> buildAllMetrics();

    /// 运行单个 per-peak 测试并返回其输出。
    /// @see ScanTestDef::PerPeak
    ScanTestOutput runPerPeakTest(const ScanTestDef &test,
                                  std::size_t &completed,
                                  std::size_t total_points,
                                  const ProgressCallback &on_progress);

    std::atomic<bool> Cancelled{false};
};

} // namespace ispp

#endif // ISPP_EXPERIMENT_SCAN_TEST_RUNNER_H
