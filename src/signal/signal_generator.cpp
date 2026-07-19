#include "ispp/signal/signal_generator.h"

namespace ispp {

/// @todo 实现信号合成流水线：
///   1. 由 SignalSpec 生成基准正弦信号（使用 Rng 传递给噪声阶段）
///   2. 若 InterferenceSpec.DeltaBins != 0，叠加干扰正弦
///   3. 按 NoiseSpec 生成对应分布噪声序列并叠加
///   4. 返回合成的 RealArray

RealArray SignalGenerator::generate(const SignalSpec &signal_spec,
                                    const EnvSpec &env_spec, Rng &rng) const {
    (void)signal_spec;
    (void)env_spec;
    (void)rng;

    return RealArray{};
}

} // namespace ispp
