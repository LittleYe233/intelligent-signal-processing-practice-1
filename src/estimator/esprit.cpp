#include "ispp/estimator/esprit.h"

namespace ispp {

/// @todo 实现 ESPRIT 频率估计算法
std::vector<FrequencyPeak>
EspritEstimator::estimate(const RealArray &input,
                          const EstimationContext &context) {
    (void)input;
    (void)context;
    return {};
}

std::string_view EspritEstimator::name() const { return "ESPRIT"; }

} // namespace ispp
