#ifndef ISPP_CORE_RNG_H
#define ISPP_CORE_RNG_H

#include <cstdint>
#include <random>

namespace ispp {

/// 统一 RNG 封装，支持种子复现。
class Rng {
public:
    explicit Rng(std::uint64_t seed);

    /// @todo 实现各分布抽样接口：
    ///   double normal(double mean, double stddev);
    ///   double uniform(double lo, double hi);
    ///   double laplace(double mean, double scale);
    ///   double impulse(double p, double magnitude);

private:
    std::mt19937_64 Engine;
};

} // namespace ispp

#endif // ISPP_CORE_RNG_H
