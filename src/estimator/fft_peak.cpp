#include "ispp/estimator/fft_peak.h"
#include "ispp/core/types.h"
#include <pocketfft_hdronly.h>

#include <algorithm>
#include <complex>
#include <cstddef>
#include <exception>
#include <vector>

namespace {

void addPeaks(
    const std::vector<std::complex<double>> &dft,
    std::vector<ispp::FrequencyPeak> &peaks,
    double threshold_factor, // NOLINT(bugprone-easily-swappable-parameters)
    double fft_rez, std::size_t max_peak_count) {
    std::vector<double> mag(dft.size());
    double max_mag = 0.0;
    for (std::size_t k = 0; k < dft.size(); ++k) {
        mag[k] = std::abs(dft[k]);
        max_mag = std::max(max_mag, mag[k]);
    }

    const double THRESHOLD = max_mag * threshold_factor;
    for (std::size_t k = 1; k < dft.size() - 1; ++k) {
        if (peaks.size() >= max_peak_count)
            break;
        const double LEFT = mag[k] - mag[k - 1];
        const double RIGHT = mag[k] - mag[k + 1];
        if (LEFT >= THRESHOLD && RIGHT >= THRESHOLD) {
            peaks.push_back(ispp::FrequencyPeak{
                .FrequencyHz = fft_rez * static_cast<double>(k),
                .Amplitude = mag[k]});
        }
    }
}

} // namespace

namespace ispp {

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
FftPeakEstimator::FftPeakEstimator(std::size_t max_peak_count, double threshold)
    : MaxPeakCount(max_peak_count), Threshold(threshold) {}

std::vector<FrequencyPeak> FftPeakEstimator::estimate(const RealArray &input,
                                                      double sample_rate) {
    const double REZ = sample_rate / static_cast<double>(input.size());
    const std::size_t OUT_SIZE = input.size() / 2 + 1;

    std::vector<std::complex<double>> spectrum(OUT_SIZE);
    std::vector<FrequencyPeak> peaks;

    const pocketfft::shape_t SHAPE = {input.size()};
    const pocketfft::stride_t STRIDE_IN = {
        static_cast<std::ptrdiff_t>(sizeof(double))};
    const pocketfft::stride_t STRIDE_OUT = {
        static_cast<std::ptrdiff_t>(sizeof(std::complex<double>))};

    try {
        pocketfft::r2c(SHAPE, STRIDE_IN, STRIDE_OUT,
                       static_cast<std::size_t>(0), pocketfft::FORWARD,
                       input.data(), spectrum.data(),
                       2.0 / static_cast<double>(input.size()));
    } catch (const std::exception &) {
        return peaks;
    }

    addPeaks(spectrum, peaks, Threshold, REZ, MaxPeakCount);
    return peaks;
}

std::string_view FftPeakEstimator::name() const { return "FFT Peak"; }

} // namespace ispp
