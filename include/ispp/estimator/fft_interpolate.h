#ifndef ISPP_ESTIMATOR_FFT_INTERPOLATE_H
#define ISPP_ESTIMATOR_FFT_INTERPOLATE_H

#include "ispp/estimator/estimator.h"

#include <cstddef>
#include <string_view>

namespace ispp {

/// 基于 FFT 插值（抛物线 / Quinn）的频率估计算法。
/// 在 FFT 幅度谱峰值附近进行插值以突破 bin 分辨率限制。
class FftInterpolateEstimator final : public IEstimator {
public:
    explicit FftInterpolateEstimator(std::size_t max_peak_count = 1,
                                     double threshold = 0.0);

    /// @todo 实现 FFT 插值频率估计
    ///   1. 对 input 做 FFT 得到幅度谱
    ///   2. 找到局部极大值后，对邻近点做抛物线或 Quinn 插值
    ///   3. 返回插值修正后的频率-幅度对列表
    std::vector<FrequencyPeak> estimate(const RealArray &input,
                                        double sample_rate) override;

    std::string_view name() const override;

private:
    std::size_t MaxPeakCount;
    double Threshold;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_FFT_INTERPOLATE_H
