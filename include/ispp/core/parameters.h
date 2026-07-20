#ifndef ISPP_CORE_PARAMETERS_H
#define ISPP_CORE_PARAMETERS_H

#include "ispp/core/types.h"

#include <cstdint>

namespace ispp {

// ---------------------------------------------------------------------------
// 原始单频正弦信号描述
// ---------------------------------------------------------------------------
struct SignalSpec {
    double SampleRateHz;
    std::size_t SampleCount;
    double FrequencyHz;
    double Amplitude;
    double PhaseRad;
};

// ---------------------------------------------------------------------------
// 窗函数
// ---------------------------------------------------------------------------
enum class WindowKind : std::uint8_t {
    RECTANGULAR,
    HAMMING,
    HANN,
    BLACKMAN,
};

struct WindowSpec {
    WindowKind Kind;
};

// ---------------------------------------------------------------------------
// 噪声
// ---------------------------------------------------------------------------
enum class NoiseDistribution : std::uint8_t {
    GAUSSIAN,
    UNIFORM,
    LAPLACIAN,
    IMPULSE, // 椒盐噪声
};

struct NoiseSpec {
    NoiseDistribution Distribution;
    double SnrDb; // 信噪比 [dB]
};

// ---------------------------------------------------------------------------
// 噪声上下文摘要（供 estimator 接口使用）
// ---------------------------------------------------------------------------
/// 与 NoiseSpec 分离：NoiseSpec 是噪声生成配置，NoiseInfo 是估计上下文，
/// 仅包含 estimator 需要知晓的最基本信息。
/// 预留扩展：未来可按分布携带额外数值参数（方差、尺度等）。
struct NoiseInfo {
    NoiseDistribution Distribution;
    double SnrDb; // 信噪比 [dB]
};

// ---------------------------------------------------------------------------
// 干扰信号
// ---------------------------------------------------------------------------
struct InterferenceSpec {
    double DeltaBins; // 与原始信号频率差（bin）；== 0 表示无干扰
    double Amplitude;
};

// ---------------------------------------------------------------------------
// 环境聚合（5 个正交维度）
// ---------------------------------------------------------------------------
struct EnvSpec {
    WindowSpec Window;
    NoiseSpec Noise;
    InterferenceSpec Interference;
};

} // namespace ispp

#endif // ISPP_CORE_PARAMETERS_H
