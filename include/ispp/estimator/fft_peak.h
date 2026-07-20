#ifndef ISPP_ESTIMATOR_FFT_PEAK_H
#define ISPP_ESTIMATOR_FFT_PEAK_H

#include "ispp/estimator/estimator.h"

#include <cstddef>
#include <string_view>

namespace ispp {

/// 基于 FFT 直接峰值搜索的频率估计算法。
/// 对输入信号进行 FFT，取幅度谱中前 max_peak_count
/// 个局部极大值作为频率估计结果。
class FftPeakEstimator final : public IEstimator {
public:
    explicit FftPeakEstimator(std::size_t max_peak_count = 1,
                              double threshold = 0.0);

    /// @todo 实现 FFT 直接峰值估计
    ///   1. 对 input 做 FFT 得到幅度谱 (可通过 PocketFFT 完成)
    ///   2. 以 threshold 过滤噪声，取前 max_peak_count 个局部极大值
    ///   3. 返回频率-幅度对列表
    std::vector<FrequencyPeak> estimate(const RealArray &input,
                                        double sample_rate) override;

    std::string_view name() const override;

private:
    std::size_t MaxPeakCount;
    double Threshold;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_FFT_PEAK_H
