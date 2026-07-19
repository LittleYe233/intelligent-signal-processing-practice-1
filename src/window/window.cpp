#include "ispp/window/window.h"

namespace ispp {

/// @todo 实现四种窗函数系数
///   Rectangular: no-op
///   Hamming:  w[n] = 0.54 - 0.46 * cos(2*pi*n/N)
///   Hann:     w[n] = 0.5  - 0.5  * cos(2*pi*n/N)
///   Blackman: w[n] = 0.42 - 0.5*cos(2*pi*n/N) + 0.08*cos(4*pi*n/N)
void applyWindow(RealArray &signal, WindowKind kind) {
    (void)signal;
    (void)kind;
}

} // namespace ispp
