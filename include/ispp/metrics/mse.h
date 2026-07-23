#ifndef ISPP_METRICS_MSE_H
#define ISPP_METRICS_MSE_H

#include "ispp/metrics/metric.h"

#include <string_view>

namespace ispp {

/// 均方误差指标：单次返回 (Δf)²，蒙特卡洛均值即为 MSE = 1/M·Σ(Δf)²。
/// 选峰策略 = max-Prominence（OQ-21），而非 min-error。
class MseMetric final : public IMetric {
public:
    double evaluate(double true_frequency_hz,
                    const EstimationResult &result) override;

    std::string_view name() const override;

    std::string format(double value) const override;

    bool showDistribution() const override { return false; }
};

} // namespace ispp

#endif // ISPP_METRICS_MSE_H
