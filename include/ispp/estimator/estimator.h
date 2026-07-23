#ifndef ISPP_ESTIMATOR_ESTIMATOR_H
#define ISPP_ESTIMATOR_ESTIMATOR_H

#include "ispp/core/parameters.h"
#include "ispp/core/types.h"
#include <cstddef>
#include <string_view>
#include <vector>

namespace ispp {

// ---------------------------------------------------------------------------
// 单次估计调用的上下文
// ---------------------------------------------------------------------------
/// 封装 estimator 需要知晓但无法从 input 推导的信号信息。
struct EstimationContext {
    double SampleRateHz;        // 采样率 [Hz]
    WindowKind WindowKind;      // input 上的窗函数类型
    std::size_t FrequencyCount; // 信号频率分量数
    NoiseInfo NoiseInfo;        // 噪声分布 + 信噪比数值信息
};

// ---------------------------------------------------------------------------
// 频率估计算法统一接口
// ---------------------------------------------------------------------------
/// 输入：已加窗的实数信号 + 上下文（采样率/窗类型/频率数/噪声信息）。
/// 算法内部不得依赖"真实频率"。
/// 返回频率-幅度对列表（由 Runner 组装为 EstimationResult 并注入计时）。
class IEstimator {
public:
    IEstimator() = default;
    IEstimator(const IEstimator &) = delete;
    IEstimator &operator=(const IEstimator &) = delete;
    IEstimator(IEstimator &&) = delete;
    IEstimator &operator=(IEstimator &&) = delete;
    virtual ~IEstimator() = default;

    /// @brief 对已加窗的 input 进行频率估计。
    ///
    /// @param input   已加窗的实数时域序列。
    /// @param context 上下文（采样率、窗类型、频率数、噪声信息）。
    /// @return 频率-幅度对列表（不含计时）。
    virtual std::vector<FrequencyPeak>
    estimate(const RealArray &input, const EstimationContext &context) = 0;

    /// 返回算法名称（用于结果显示）。
    virtual std::string_view name() const = 0;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_ESTIMATOR_H
