#include "ispp/estimator/fft_interpolate.h"

namespace ispp {

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
FftInterpolateEstimator::FftInterpolateEstimator(std::size_t max_peak_count,
                                                 double threshold)
    : MaxPeakCount(max_peak_count), Threshold(threshold) {}

/// @todo 实现 FFT 插值频率估计：
///   1. 使用 PocketFFT 对 input 进行 r2c 变换
///   2. 计算单边幅度谱，找到局部极大值（幅度 > Threshold）
///   3. 对每个峰附近的 3 个点做抛物线插值或 Quinn 插值
///   4. 取前 MaxPeakCount 个修正后的频率-幅度对
///   5. 返回频率-幅度对列表
std::vector<FrequencyPeak>
FftInterpolateEstimator::estimate(const RealArray &input, double sample_rate) {
    (void)input;
    (void)sample_rate;

    return {};
}

std::string_view FftInterpolateEstimator::name() const {
    return "FFT Interpolate";
}

} // namespace ispp
