#include "ispp/estimator/fft_interpolate.h"

namespace ispp {

FftInterpolateEstimator::FftInterpolateEstimator(double threshold)
    : Threshold(threshold) {}

/// @todo 实现 FFT 插值频率估计：
///   1. 使用 computeDft(input) 得到单边复数谱
///   2. 使用 findPeaksFromDft(dft, Threshold, bin_hz,
///      context.frequencyCount) 得粗峰
///   3. 按 context.windowKind 选择插值算法：
///        - RECTANGULAR / 未知：通用抛物线或 Quinn 插值
///        - HANN / HAMMING / BLACKMAN：窗特定系数修正
///   4. 对每个粗峰邻近 3 点做插值，修正频率与幅度
///   5. 返回修正后的频率-幅度对列表
std::vector<FrequencyPeak>
FftInterpolateEstimator::estimate(const RealArray &input,
                                  const EstimationContext &context) {
    (void)input;
    (void)context;

    return {};
}

std::string_view FftInterpolateEstimator::name() const {
    return "FFT Interpolate";
}

} // namespace ispp
