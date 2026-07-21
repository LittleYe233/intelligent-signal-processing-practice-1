#ifndef ISPP_SIGNAL_SIGNAL_GENERATOR_H
#define ISPP_SIGNAL_SIGNAL_GENERATOR_H

#include "ispp/core/parameters.h"
#include "ispp/core/rng.h"
#include "ispp/core/types.h"

namespace ispp {

/// 输入信号生成器。
/// 流水线：原始正弦 → (+干扰) → (+噪声) → 输出实数信号。
class SignalGenerator {
public:
    /// 合成"输入信号" = 原始正弦 + (可选)干扰 + 噪声。
    /// 噪声幅值按 SNR 与信号 RMS 自动缩放。
    RealArray generate(const SignalSpec &signal, const EnvSpec &env,
                       Rng &rng) const;
};

} // namespace ispp

#endif // ISPP_SIGNAL_SIGNAL_GENERATOR_H
