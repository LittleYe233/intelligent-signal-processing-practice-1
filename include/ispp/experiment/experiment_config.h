#ifndef ISPP_EXPERIMENT_EXPERIMENT_CONFIG_H
#define ISPP_EXPERIMENT_EXPERIMENT_CONFIG_H

#include "ispp/core/parameters.h"

#include <cstddef>
#include <cstdint>

namespace ispp {

struct MonteCarloConfig {
    std::size_t IterationCount; // 蒙特卡洛迭代次数（默认 100）
    std::uint64_t BaseSeed;     // 基准种子；第 i 次使用 BaseSeed + i
};

struct ExperimentConfig {
    SignalSpec Signal;
    EnvSpec Env;
    MonteCarloConfig MonteCarlo;
    std::size_t MaxFreqCount; // 用于 MUSIC/ESPRIT 的最大频率数
};

} // namespace ispp

#endif // ISPP_EXPERIMENT_EXPERIMENT_CONFIG_H
