#include "ispp/signal/signal_generator.h"

#include <cmath>
#include <numbers>
#include <vector>

namespace ispp {

RealArray SignalGenerator::generate(const SignalSpec &signal,
                                    const EnvSpec &env, Rng &rng) const {
    const std::size_t N = signal.SampleCount;
    const double FS = signal.SampleRateHz;
    const double AMP = signal.Amplitude;
    const double FREQ = signal.FrequencyHz;
    const double PHASE = signal.PhaseRad;

    std::vector<double> buf(N);

    // -----------------------------------------------------------------------
    // 1. 原始正弦：A * sin(2π * f/fs * n + φ)
    // -----------------------------------------------------------------------
    for (std::size_t n = 0; n < N; ++n) {
        const double T = static_cast<double>(n) / FS;
        buf[n] = AMP * std::sin(2.0 * std::numbers::pi * FREQ * T + PHASE);
    }

    // -----------------------------------------------------------------------
    // 2. 干扰正弦：若 DeltaBins != 0，叠加到 buf
    // -----------------------------------------------------------------------
    const double DELTA = env.Interference.DeltaBins;
    if (DELTA != 0.0) {
        const double BIN_HZ = FS / static_cast<double>(N);
        const double INT_FREQ = FREQ + DELTA * BIN_HZ;
        const double INT_AMP = env.Interference.Amplitude;
        for (std::size_t n = 0; n < N; ++n) {
            const double T = static_cast<double>(n) / FS;
            buf[n] += INT_AMP * std::sin(2.0 * std::numbers::pi * INT_FREQ * T);
        }
    }

    // -----------------------------------------------------------------------
    // 3. 噪声：按分布与 SNR 叠加
    // -----------------------------------------------------------------------
    // 信号 RMS = A / √2，信号功率 = A² / 2
    // 噪声功率 = 信号功率 / 10^(SNR/10)
    // 噪声标准差 = √(噪声功率)
    const double SNR_DB = env.Noise.SnrDb;
    const double SIG_RMS = AMP / std::numbers::sqrt2;
    const double NOISE_POWER =
        (SIG_RMS * SIG_RMS) / std::pow(10.0, SNR_DB / 10.0);
    const double NOISE_STD = std::sqrt(NOISE_POWER);

    switch (env.Noise.Distribution) {
    case NoiseDistribution::GAUSSIAN:
        for (std::size_t n = 0; n < N; ++n) {
            buf[n] += rng.normal(0.0, NOISE_STD);
        }
        break;

    case NoiseDistribution::UNIFORM: {
        const double A = NOISE_STD * std::numbers::sqrt3;
        for (std::size_t n = 0; n < N; ++n) {
            buf[n] += rng.uniform(-A, A);
        }
        break;
    }

    case NoiseDistribution::LAPLACIAN: {
        const double B = NOISE_STD / std::numbers::sqrt2;
        for (std::size_t n = 0; n < N; ++n) {
            buf[n] += rng.laplace(0.0, B);
        }
        break;
    }

    case NoiseDistribution::IMPULSE: {
        const double P = 0.1;
        const double MAG = NOISE_STD / std::sqrt(P * (1.0 - P));
        for (std::size_t n = 0; n < N; ++n) {
            buf[n] += rng.impulse(P, MAG);
        }
        break;
    }
    }

    return buf;
}

} // namespace ispp
