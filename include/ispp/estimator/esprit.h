#ifndef ISPP_ESTIMATOR_ESPRIT_H
#define ISPP_ESTIMATOR_ESPRIT_H

#include "ispp/estimator/estimator.h"

#include <string_view>
#include <vector>

namespace ispp {

/// ESPRIT 频率估计算法（用户实现）。
class EspritEstimator final : public IEstimator {
public:
    EspritEstimator() = default;

    std::vector<FrequencyPeak>
    estimate(const RealArray &input, const EstimationContext &context) override;

    std::string_view name() const override;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_ESPRIT_H
