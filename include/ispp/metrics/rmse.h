#ifndef ISPP_METRICS_RMSE_H
#define ISPP_METRICS_RMSE_H

#include "ispp/metrics/metric.h"

#include <string_view>

namespace ispp {

/// 均方误差指标：单次返回 (Δf)²，蒙特卡洛均值即为 MSE。
/// 多峰时选取与真实频率误差最小的峰（OQ-6）。
class RmseMetric final : public IMetric {
public:
    double evaluate(double true_frequency_hz,
                    const EstimationResult &result) override;

    std::string_view name() const override;
};

} // namespace ispp

#endif // ISPP_METRICS_RMSE_H
