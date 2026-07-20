#ifndef ISPP_ESTIMATOR_FFT_PEAK_H
#define ISPP_ESTIMATOR_FFT_PEAK_H

#include "ispp/estimator/estimator.h"

#include <string_view>
#include <vector>

namespace ispp {

/// 基于 FFT 直接峰值搜索的频率估计算法。
/// 对输入信号进行 FFT，取幅度谱中前 context.frequencyCount
/// 个局部极大值作为频率估计结果。
class FftPeakEstimator final : public IEstimator {
public:
    explicit FftPeakEstimator(double threshold = 0.0);

    /// @brief FFT 直接峰值估计（委托 core/fft 公共工具）。
    std::vector<FrequencyPeak>
    estimate(const RealArray &input, const EstimationContext &context) override;

    std::string_view name() const override;

private:
    double Threshold;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_FFT_PEAK_H
