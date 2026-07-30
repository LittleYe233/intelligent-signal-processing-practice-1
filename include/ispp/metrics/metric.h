#ifndef ISPP_METRICS_METRIC_H
#define ISPP_METRICS_METRIC_H

#include "ispp/core/parameters.h" // NoiseInfo
#include "ispp/core/types.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

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
    /// 多峰时各指标有自己的选峰策略（OQ-6 / OQ-21）。
    /// 聚合指标（isAggregate() == true）不需实现有效逻辑——
    /// Runner 不会调用此方法。
    virtual double evaluate(double true_frequency_hz,
                            const EstimationResult &result) = 0;

    /// 返回指标的英语 msgid 字面量——既是 locale 无关的身份键（供 worker 线程
    /// 比较匹配），也是 gettext 翻译键。结果显示层在 UI 线程经 `_UI(name())`
    /// 本地化。**不得**在此调用 `_UI()`/`dgettext()`（本方法会被 worker
    /// 线程调用）。
    virtual std::string_view name() const = 0;

    /// 将统计值格式化为人类可读字符串（决策记录 OQ-13）。
    virtual std::string format(double value) const = 0;

    /// 是否在结果面板展示完整的统计分布（mean/std/min/max）。
    /// MSE 和 RelativeEfficiency 返回 false（仅显示单一值）。
    virtual bool showDistribution() const { return true; }

    // --- 聚合指标扩展（OQ-20）---------------------------------------------

    /// 是否为聚合指标（蒙特卡洛结束后一次性计算，而非每轮迭代评估）。
    /// 聚合指标使用 finalize() 而非 evaluate() + computeStats()。
    virtual bool isAggregate() const { return false; }

    /// 聚合指标的后处理计算（MC 循环结束后调用一次）。
    /// 默认实现返回 0.0（非聚合指标不覆写此方法）。
    virtual double finalize(const std::vector<double> &freq_estimates,
                            double sample_rate_hz, std::size_t sample_count,
                            NoiseInfo noise_info) const {
        (void)freq_estimates;
        (void)sample_rate_hz;
        (void)sample_count;
        (void)noise_info;
        return 0.0;
    }
};

} // namespace ispp

#endif // ISPP_METRICS_METRIC_H
