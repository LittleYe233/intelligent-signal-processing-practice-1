#include "ispp/window/window.h"
#include <cmath>

namespace ispp {

void applyWindow(RealArray &signal, WindowKind kind) {
    const size_t N = signal.size();
    switch (kind) {
    case WindowKind::RECTANGULAR:
        break;
    case WindowKind::HAMMING:
        for (size_t n = 0; n < N; ++n) {
            signal[n] *= 0.54 - 0.46 * std::cos(2 * std::numbers::pi *
                                                static_cast<double>(n) /
                                                static_cast<double>(N));
        }
        break;
    case WindowKind::HANN:
        for (size_t n = 0; n < N; ++n) {
            signal[n] *= 0.5 - 0.5 * std::cos(2 * std::numbers::pi *
                                              static_cast<double>(n) /
                                              static_cast<double>(N));
        }
        break;
    case WindowKind::BLACKMAN:
        for (size_t n = 0; n < N; ++n) {
            signal[n] *=
                0.42 -
                0.5 * std::cos(2 * std::numbers::pi * static_cast<double>(n) /
                               static_cast<double>(N)) +
                0.08 * std::cos(4 * std::numbers::pi * static_cast<double>(n) /
                                static_cast<double>(N));
        }
        break;
    }
}

} // namespace ispp
