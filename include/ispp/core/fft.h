#ifndef ISPP_CORE_FFT_H
#define ISPP_CORE_FFT_H

#include "ispp/core/types.h"

#include <cstddef>
#include <vector>

namespace ispp {

/// @brief 对实数序列做 r2c FFT，返回单边复数谱。
///
/// 输出长度为 `N/2+1`（N 为 input 长度）。幅度已按单边谱归一化：
/// 每个 bin 乘以 `2.0 / N`，使正弦信号峰值幅度近似等于时域幅度。
///
/// @param input 实数时域采样序列（通常已加窗）。
/// @return 单边复数 DFT；FFT 失败时返回空数组。
ComplexArray computeDft(const RealArray &input);

/// @brief 从复数 DFT 幅度谱中提取局部极大值峰值。
///
/// 对每个 bin 计算幅度，以 `max_mag * threshold_factor` 为阈值，
/// 取前 `max_peak_count` 个严格局部极大值（左右邻点均低于当前点至少
/// 一个阈值量）。频率 = `bin_index * bin_hz`。
///
/// @param dft 单边复数 DFT（通常来自 computeDft）。
/// @param threshold_factor 相对最大幅值的比率阈值，范围建议 [0, 1]。
/// @param bin_hz 频率分辨率 Hz/bin（= sample_rate / N）。
/// @param max_peak_count 最多返回的峰值个数。
/// @return 频率-幅度对列表；dft 为空时返回空列表。
std::vector<FrequencyPeak> findPeaksFromDft(const ComplexArray &dft,
                                            double threshold_factor,
                                            double bin_hz,
                                            std::size_t max_peak_count);

} // namespace ispp

#endif // ISPP_CORE_FFT_H
