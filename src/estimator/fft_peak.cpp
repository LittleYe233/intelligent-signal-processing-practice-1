#include "ispp/estimator/fft_peak.h"

#include "ispp/core/fft.h"

namespace ispp {

FftPeakEstimator::FftPeakEstimator(std::size_t max_peak_count, double threshold)
    : MaxPeakCount(max_peak_count), Threshold(threshold) {}

std::vector<FrequencyPeak> FftPeakEstimator::estimate(const RealArray &input,
                                                      double sample_rate,
                                                      WindowKind window_kind) {
    (void)window_kind; // 直接峰值搜索不依赖窗类型

    if (input.empty()) {
        return {};
    }

    const ComplexArray DFT = computeDft(input);
    if (DFT.empty()) {
        return {};
    }

    const double BIN_HZ = sample_rate / static_cast<double>(input.size());
    return findPeaksFromDft(DFT, Threshold, BIN_HZ, MaxPeakCount);
}

std::string_view FftPeakEstimator::name() const { return "FFT Peak"; }

} // namespace ispp
