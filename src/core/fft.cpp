#include "ispp/core/fft.h"
#include "ispp/core/peak_finder.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <exception>
#include <pocketfft_hdronly.h>
#include <vector>

namespace ispp {

ComplexArray computeDft(const RealArray &input) {
    if (input.empty()) {
        return {};
    }

    const std::size_t N = input.size();
    const std::size_t OUT_SIZE = N / 2 + 1;
    ComplexArray spectrum(OUT_SIZE);

    const pocketfft::shape_t SHAPE = {N};
    const pocketfft::stride_t STRIDE_IN = {
        static_cast<std::ptrdiff_t>(sizeof(double))};
    const pocketfft::stride_t STRIDE_OUT = {
        static_cast<std::ptrdiff_t>(sizeof(std::complex<double>))};

    try {
        pocketfft::r2c(SHAPE, STRIDE_IN, STRIDE_OUT,
                       static_cast<std::size_t>(0), pocketfft::FORWARD,
                       input.data(), spectrum.data(),
                       2.0 / static_cast<double>(N));
    } catch (const std::exception &) {
        return {};
    }

    return spectrum;
}

std::vector<FrequencyPeak> findPeaksFromDft(const ComplexArray &dft,
                                            double threshold_factor,
                                            double bin_hz,
                                            std::size_t max_peak_count) {
    if (dft.size() < 3 || max_peak_count == 0) {
        return {};
    }

    // 1. Compute magnitude array
    std::vector<double> mags(dft.size());
    double max_mag = 0.0;
    for (std::size_t k = 0; k < dft.size(); ++k) {
        mags[k] = std::abs(dft[k]);
        max_mag = std::max(max_mag, mags[k]);
    }

    // 2. Map old threshold_factor-based parameters to PeakFinder parameters
    const std::size_t KERNEL_SIZE = 31;
    const double MARGIN = threshold_factor * max_mag;
    const double MIN_PROMINENCE = threshold_factor * max_mag;
    const double MIN_WIDTH = 1.0;

    // 3. Delegate to PeakFinder<double>
    auto pf_peaks = PeakFinder<double>::findPeaks(mags, KERNEL_SIZE, MARGIN,
                                                  MIN_PROMINENCE, MIN_WIDTH);

    // 4. Apply max_peak_count trimming (keep top N by prominence)
    if (pf_peaks.size() > max_peak_count) {
        std::ranges::partial_sort(
            pf_peaks,
            pf_peaks.begin() + static_cast<std::ptrdiff_t>(max_peak_count),
            [](const auto &a, const auto &b) {
                return a.Prominence > b.Prominence;
            });
        pf_peaks.resize(max_peak_count);
        // Restore index-ascending order after prominence-sort
        std::ranges::sort(pf_peaks, {}, [](const auto &p) { return p.Index; });
    }

    // 5. Map PeakFinder::Peak → FrequencyPeak
    std::vector<FrequencyPeak> result;
    result.reserve(pf_peaks.size());
    for (const auto &p : pf_peaks) {
        result.push_back(
            FrequencyPeak{.FrequencyHz = static_cast<double>(p.Index) * bin_hz,
                          .Amplitude = mags[p.Index]});
    }

    return result;
}

} // namespace ispp
