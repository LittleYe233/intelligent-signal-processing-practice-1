#ifndef ISPP_METRICS_RELATIVE_EFFICIENCY_H
#define ISPP_METRICS_RELATIVE_EFFICIENCY_H

#include "ispp/metrics/metric.h"

#include <string_view>

namespace ispp {

/// 相对效率指标（聚合）：η = CRB / SampleVariance。
/// 仅高斯/拉普拉斯分布有 CRB 解析式；均匀/脉冲分布返回 NaN。
/// 选峰策略 = max-Prominence（OQ-21），与 MSE 一致。
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
