#ifndef ISPP_ESTIMATOR_ESTIMATOR_H
#define ISPP_ESTIMATOR_ESTIMATOR_H

#include "ispp/core/parameters.h"
#include "ispp/core/types.h"

#include <string_view>
#include <vector>

namespace ispp {

/// 频率估计算法统一接口。
/// 输入：已加窗的实数信号幅度序列 + 采样率 + 窗函数类型；
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
    /// @param input 已加窗的实数时域序列。
    /// @param sample_rate 采样率 [Hz]。
    /// @param window_kind 施加在 input 上的窗函数类型；默认 RECTANGULAR。
    ///        不关心窗类型的算法可忽略；未知/不支持的窗由算法内部回退处理。
    /// @return 频率-幅度对列表（不含计时）。
    virtual std::vector<FrequencyPeak>
    estimate(const RealArray &input, double sample_rate,
             WindowKind window_kind = WindowKind::RECTANGULAR) = 0;

    /// 返回算法名称（用于结果显示）
    virtual std::string_view name() const = 0;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_ESTIMATOR_H
