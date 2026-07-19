#ifndef ISPP_WINDOW_WINDOW_H
#define ISPP_WINDOW_WINDOW_H

#include "ispp/core/parameters.h"
#include "ispp/core/types.h"

namespace ispp {

/// 对输入信号信号就地施加窗函数。
/// @todo 实现四种窗系数：矩形(no-op)、Hamming、Hann、Blackman
void applyWindow(RealArray &signal, WindowKind kind);

} // namespace ispp

#endif // ISPP_WINDOW_WINDOW_H
