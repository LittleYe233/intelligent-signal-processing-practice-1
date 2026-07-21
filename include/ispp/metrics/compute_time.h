#ifndef ISPP_METRICS_COMPUTE_TIME_H
#define ISPP_METRICS_COMPUTE_TIME_H

#include "ispp/metrics/metric.h"

#include <string_view>

namespace ispp {

/// 计算耗时指标：直接返回 result.ComputeTimeSec。
class ComputeTimeMetric final : public IMetric {
public:
    double evaluate(double true_frequency_hz,
                    const EstimationResult &result) override;

    std::string_view name() const override;
};

} // namespace ispp

#endif // ISPP_METRICS_COMPUTE_TIME_H
