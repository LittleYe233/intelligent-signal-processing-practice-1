#ifndef ISPP_ESTIMATOR_FFT_PEAK_H
#define ISPP_ESTIMATOR_FFT_PEAK_H

#include "ispp/estimator/estimator.h"

#include <cstddef>
#include <string_view>
#include <vector>

namespace ispp {

/// 基于 FFT 直接峰值搜索的频率估计算法。
/// 对输入信号进行 FFT，取幅度谱中前 max_peak_count
/// 个局部极大值作为频率估计结果。
class FftPeakEstimator final : public IEstimator {
public:
    explicit FftPeakEstimator(std::size_t max_peak_count = 1,
                              double threshold = 0.0);

    /// @brief FFT 直接峰值估计（委托 core/fft 公共工具）。
    /// @param window_kind 本算法不依赖窗类型，参数被忽略。
    std::vector<FrequencyPeak> estimate(const RealArray &input,
                                        double sample_rate,
                                        WindowKind window_kind) override;

    std::string_view name() const override;

private:
    std::size_t MaxPeakCount;
    double Threshold;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_FFT_PEAK_H
