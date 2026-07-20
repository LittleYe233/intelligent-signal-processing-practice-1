#include "ispp/estimator/fft_interpolate.h"

namespace ispp {

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
FftInterpolateEstimator::FftInterpolateEstimator(std::size_t max_peak_count,
                                                 double threshold)
    : MaxPeakCount(max_peak_count), Threshold(threshold) {}

/// @todo 实现 FFT 插值频率估计：
///   1. 使用 computeDft(input) 得到单边复数谱
///   2. 使用 findPeaksFromDft(dft, Threshold, bin_hz, MaxPeakCount) 得粗峰
///   3. 按 window_kind 选择插值算法：
///        - RECTANGULAR / 未知：通用抛物线或 Quinn 插值
///        - HANN / HAMMING / BLACKMAN：窗特定系数修正
///   4. 对每个粗峰邻近 3 点做插值，修正频率与幅度
///   5. 返回修正后的频率-幅度对列表
std::vector<FrequencyPeak>
FftInterpolateEstimator::estimate(const RealArray &input, double sample_rate,
                                  WindowKind window_kind) {
    (void)input;
    (void)sample_rate;
    (void)window_kind;

    return {};
}

std::string_view FftInterpolateEstimator::name() const {
    return "FFT Interpolate";
}

} // namespace ispp
