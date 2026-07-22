#ifndef ISPP_METRICS_METRIC_H
#define ISPP_METRICS_METRIC_H

#include "ispp/core/types.h"

#include <string>
#include <string_view>

namespace ispp {

/// 评价指标统一接口。
class IMetric {
public:
    IMetric() = default;
    IMetric(const IMetric &) = delete;
    IMetric &operator=(const IMetric &) = delete;
    IMetric(IMetric &&) = delete;
    IMetric &operator=(IMetric &&) = delete;
    virtual ~IMetric() = default;

    /// 单次评估：比较真实频率与估计结果，返回指标值。
    /// 多峰时选取误差最小的峰（决策记录 OQ-6）。
    virtual double evaluate(double true_frequency_hz,
                            const EstimationResult &result) = 0;

    /// 返回指标名称（用于结果显示）。
    virtual std::string_view name() const = 0;

    /// 将统计值格式化为人类可读字符串（决策记录 OQ-13）。
    virtual std::string format(double value) const = 0;

    /// 是否在结果面板展示完整的统计分布（mean/std/min/max）。
    /// RMSE 返回 false（仅显示单一 RMSE 值），其余返回 true。
    virtual bool showDistribution() const { return true; }
};

} // namespace ispp

#endif // ISPP_METRICS_METRIC_H
