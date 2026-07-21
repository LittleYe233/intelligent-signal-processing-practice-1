#ifndef ISPP_CORE_RNG_H
#define ISPP_CORE_RNG_H

#include <cstdint>
#include <random>

namespace ispp {

/// 统一 RNG 封装，支持种子复现（蒙特卡洛可重现）。
class Rng {
public:
    explicit Rng(std::uint64_t seed);

    /// 高斯分布 ~ N(mean, stddev^2)
    double normal(double mean, double stddev);

    /// 均匀分布 ~ U(lo, hi)
    double uniform(double lo, double hi);

    /// 拉普拉斯分布（双指数分布），尺度参数 scale
    double laplace(double mean, double scale);

    /// 脉冲噪声：以概率 p 返回 magnitude，否则返回 0
    double impulse(double p, double magnitude);

private:
    std::mt19937_64 Engine;
};

} // namespace ispp

#endif // ISPP_CORE_RNG_H
