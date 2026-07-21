#ifndef ISPP_WINDOW_WINDOW_H
#define ISPP_WINDOW_WINDOW_H

#include "ispp/core/parameters.h"
#include "ispp/core/types.h"

namespace ispp {

/// 对输入信号信号就地施加窗函数。
///
///   Rectangular: no-op
///
///   Hamming:  w[n] = 0.54 - 0.46 * cos(2*pi*n/N)
///
///   Hann:     w[n] = 0.5  - 0.5  * cos(2*pi*n/N)
///
///   Blackman: w[n] = 0.42 - 0.5*cos(2*pi*n/N) + 0.08*cos(4*pi*n/N)
void applyWindow(RealArray &signal, WindowKind kind);

} // namespace ispp

#endif // ISPP_WINDOW_WINDOW_H
