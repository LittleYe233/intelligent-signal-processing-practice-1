#ifndef ISPP_METRICS_RELATIVE_EFFICIENCY_H
#define ISPP_METRICS_RELATIVE_EFFICIENCY_H

#include "ispp/metrics/metric.h"

#include <string_view>

namespace ispp {

/// 相对效率指标（聚合）：η = CRB / SampleVariance。
/// 仅高斯/拉普拉斯分布有 CRB 解析式；均匀/脉冲分布返回 NaN。
/// 选峰策略 = max-Prominence（OQ-21），与 MSE 一致。
///
/// @note **当前已禁用**（OQ-20）。模型假设与 CRB 正则化条件不完全匹配，
///       实际仿真场景下无法给出有意义的 η 值。代码保留以便未来修正模型后
///       快速重新启用。重新启用步骤：
///       1. 在 `config_panel.cpp` 注册 `RelativeEfficiencyMetric`；
///       2. 在 `experiment_runner.cpp` 恢复聚合指标处理逻辑
///          （`has_aggregate` / `freq_estimates` / `finalize()` 调用）。
///       文件仍参与编译以防止 API 漂移。
class RelativeEfficiencyMetric final : public IMetric {
public:
    double evaluate(double true_frequency_hz,
                    const EstimationResult &result) override;

    std::string_view name() const override;

    std::string format(double value) const override;

    bool showDistribution() const override { return false; }

    bool isAggregate() const override { return true; }

    double finalize(const std::vector<double> &freq_estimates,
                    double sample_rate_hz, std::size_t sample_count,
                    NoiseInfo noise_info) const override;
};

} // namespace ispp

#endif // ISPP_METRICS_RELATIVE_EFFICIENCY_H
