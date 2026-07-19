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
    /// @todo 实现合成逻辑
    ///   steps:
    ///     1. 由 SignalSpec 生成基准正弦
    ///     2. 若 InterferenceSpec.DeltaBins != 0，叠加干扰正弦
    ///     3. 按 NoiseSpec 生成噪声序列并叠加
    ///     4. 返回合成的 RealArray
    RealArray generate(const SignalSpec &signal_spec, const EnvSpec &env_spec,
                       Rng &rng) const;
};

} // namespace ispp

#endif // ISPP_SIGNAL_SIGNAL_GENERATOR_H
