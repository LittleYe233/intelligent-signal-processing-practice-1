#ifndef ISPP_METRICS_METRIC_H
#define ISPP_METRICS_METRIC_H

#include "ispp/core/types.h"

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

    /// @todo 实现：比较真实频率与估计结果，返回指标值
    virtual double evaluate(double true_frequency_hz,
                            const EstimationResult &result) = 0;

    /// 返回指标名称（用于结果显示）
    virtual std::string_view name() const = 0;
};

} // namespace ispp

#endif // ISPP_METRICS_METRIC_H
