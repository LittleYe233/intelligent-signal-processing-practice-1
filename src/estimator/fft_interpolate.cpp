#include "ispp/estimator/fft_interpolate.h"
#include "ispp/core/fft.h"
#include "ispp/core/types.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdlib>

namespace {

double searchCheck(const ispp::RealArray &input, const double delta) {
    const std::complex<double> COEF =
        std::polar(1.0, -2.0 * std::numbers::pi * delta /
                            static_cast<double>(input.size()));
    std::complex<double> result = 0;
    for (double it : input) {
        result += it * COEF;
    }
    return std::abs(result);
}

} // namespace

namespace ispp {

FftInterpolateEstimator::FftInterpolateEstimator(double threshold)
    : Threshold(threshold) {}

std::vector<FrequencyPeak>
FftInterpolateEstimator::estimate(const RealArray &input,
                                  const EstimationContext &context) {
    // Initial parameters
    // Consider move them to window members

    /// @note Half width of binary search (unit: bin).
    const double EPS = 0.1;
    /// @note Binary search iteration count.
    const size_t MAX_SEARCH_CNT = 5;

    if (input.empty()) {
        return {};
    }

    const ComplexArray DFT = computeDft(input);
    if (DFT.empty()) {
        return {};
    }

    const size_t N = input.size();
    const double BIN_HZ = context.SampleRateHz / static_cast<double>(N);
    auto peaks =
        findPeaksFromDft(DFT, Threshold, BIN_HZ, context.FrequencyCount);
    if (peaks.empty()) {
        return {};
    }

    // Only choose the highest peak
    const size_t MAX_PEAK_IDX =
        // NOLINTNEXTLINE(modernize-use-ranges)
        static_cast<size_t>(std::max_element(peaks.cbegin(), peaks.cend(),
                                             [](auto a, auto b) {
                                                 return a.Amplitude <
                                                        b.Amplitude;
                                             }) -
                            peaks.begin());
    const auto K_MAX = static_cast<size_t>(
        std::round(peaks[MAX_PEAK_IDX].FrequencyHz / BIN_HZ));

    // Compare left and right spectrum line
    // Edge case
    if (K_MAX == 0 || K_MAX == N / 2) {
        return std::vector<FrequencyPeak>{peaks[MAX_PEAK_IDX]};
    }
    // Peaks have been normalized, but we need other elements, so access raw DFT
    // array instead.
    const double MAG_RIGHT = std::abs(DFT[K_MAX + 1]),
                 MAG_LEFT = std::abs(DFT[K_MAX - 1]),
                 MAG_PEAK = std::abs(DFT[K_MAX]);
    const double ETA = ([=]() {
        if (MAG_RIGHT < MAG_LEFT) {
            return static_cast<double>(N) * std::numbers::inv_pi *
                   std::atan(
                       2 * std::sin(std::numbers::pi / static_cast<double>(N)) *
                       MAG_LEFT * MAG_RIGHT / (MAG_LEFT + MAG_RIGHT) /
                       MAG_PEAK);
        } else {
            return static_cast<double>(N) * std::numbers::inv_pi *
                   std::atan(
                       -2 *
                       std::sin(std::numbers::pi / static_cast<double>(N)) *
                       MAG_LEFT * MAG_RIGHT / (MAG_LEFT + MAG_RIGHT) /
                       MAG_PEAK);
        }
    })();
    // Initial delta (delta_0)
    double delta = static_cast<double>(K_MAX) - ETA;

    // Binary search
    // Initial half-width. For m-th search, half-width is EPS / 2 ** m.
    double width = EPS;
    for (size_t m = 1; m <= MAX_SEARCH_CNT; ++m) {
        width /= 2;
        const double RIGHT = delta + width, LEFT = delta - width;
        if (searchCheck(input, RIGHT) > searchCheck(input, LEFT)) {
            delta = RIGHT;
        } else {
            delta = LEFT;
        }
    }
    const double FREQ = BIN_HZ * delta;

    return std::vector<FrequencyPeak>{
        FrequencyPeak{.FrequencyHz = FREQ, .Amplitude = AMP_UNKNOWN}};
}

std::string_view FftInterpolateEstimator::name() const {
    return "FFT Interpolate";
}

} // namespace ispp
