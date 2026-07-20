#include "ispp/core/fft.h"

#include <pocketfft_hdronly.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <exception>
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
    std::vector<FrequencyPeak> peaks;
    if (dft.size() < 3 || max_peak_count == 0) {
        return peaks;
    }

    std::vector<double> mag(dft.size());
    double max_mag = 0.0;
    for (std::size_t k = 0; k < dft.size(); ++k) {
        mag[k] = std::abs(dft[k]);
        max_mag = std::max(max_mag, mag[k]);
    }

    const double THRESHOLD = max_mag * threshold_factor;
    for (std::size_t k = 1; k + 1 < dft.size(); ++k) {
        if (peaks.size() >= max_peak_count) {
            break;
        }
        const double LEFT = mag[k] - mag[k - 1];
        const double RIGHT = mag[k] - mag[k + 1];
        if (LEFT >= THRESHOLD && RIGHT >= THRESHOLD) {
            peaks.push_back(
                FrequencyPeak{.FrequencyHz = bin_hz * static_cast<double>(k),
                              .Amplitude = mag[k]});
        }
    }

    return peaks;
}

} // namespace ispp
