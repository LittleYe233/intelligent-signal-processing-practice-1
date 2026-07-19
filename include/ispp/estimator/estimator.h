#ifndef ISPP_ESTIMATOR_ESTIMATOR_H
#define ISPP_ESTIMATOR_ESTIMATOR_H

#include "ispp/core/types.h"

#include <memory>
#include <string_view>

namespace ispp {

/// 频率估计算法统一接口。
/// 输入：实数信号幅度序列 + 采样率；算法内部不得依赖"真实频率"。
class IEstimator {
public:
    IEstimator() = default;
    IEstimator(const IEstimator &) = delete;
    IEstimator &operator=(const IEstimator &) = delete;
    IEstimator(IEstimator &&) = delete;
    IEstimator &operator=(IEstimator &&) = delete;
    virtual ~IEstimator() = default;

    /// @todo 实现：对 input 进行频率估计，返回频率-幅度对列表
    virtual EstimationResult estimate(const RealArray &input,
                                      double sample_rate) = 0;

    /// 返回算法名称（用于结果显示）
    virtual std::string_view name() const = 0;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_ESTIMATOR_H
