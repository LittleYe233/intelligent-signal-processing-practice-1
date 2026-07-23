#ifndef ISPP_ESTIMATOR_MUSIC_H
#define ISPP_ESTIMATOR_MUSIC_H

#include "ispp/estimator/estimator.h"
#include <string_view>
#include <vector>

namespace ispp {

/// MUSIC 频率估计算法（用户实现）。
class MusicEstimator final : public IEstimator {
public:
    explicit MusicEstimator(double threshold = 0.0);

    std::vector<FrequencyPeak>
    estimate(const RealArray &input, const EstimationContext &context) override;

    std::string_view name() const override;

private:
    double Threshold;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_MUSIC_H
