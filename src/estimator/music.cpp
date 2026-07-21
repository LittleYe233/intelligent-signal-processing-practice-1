#include "ispp/estimator/music.h"

namespace ispp {

/// @todo 实现 MUSIC 频率估计算法
std::vector<FrequencyPeak>
MusicEstimator::estimate(const RealArray &input,
                         const EstimationContext &context) {
    (void)input;
    (void)context;
    return {};
}

std::string_view MusicEstimator::name() const { return "MUSIC"; }

} // namespace ispp
