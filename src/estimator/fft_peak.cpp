#include "ispp/estimator/fft_peak.h"
#include "ispp/core/fft.h"

namespace ispp {

FftPeakEstimator::FftPeakEstimator(double threshold) : Threshold(threshold) {}

std::vector<FrequencyPeak>
FftPeakEstimator::estimate(const RealArray &input,
                           const EstimationContext &context) {
    if (input.empty()) {
        return {};
    }

    const ComplexArray DFT = computeDft(input);
    if (DFT.empty()) {
        return {};
    }

    const double BIN_HZ =
        context.SampleRateHz / static_cast<double>(input.size());
    return findPeaksFromDft(DFT, Threshold, BIN_HZ, context.FrequencyCount);
}

std::string_view FftPeakEstimator::name() const { return "FFT Peak"; }

} // namespace ispp
