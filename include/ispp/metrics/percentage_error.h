#ifndef ISPP_METRICS_PERCENTAGE_ERROR_H
#define ISPP_METRICS_PERCENTAGE_ERROR_H

#include "ispp/metrics/metric.h"

#include <string_view>

namespace ispp {

/// 百分比误差指标：|Δf| / f_true × 100%。
/// 多峰时选取与真实频率误差最小的峰（OQ-6）。
class PercentageErrorMetric final : public IMetric {
public:
    double evaluate(double true_frequency_hz,
                    const EstimationResult &result) override;

    std::string_view name() const override;

    std::string format(double value) const override;
};

} // namespace ispp

#endif // ISPP_METRICS_PERCENTAGE_ERROR_H
