#ifndef ISPP_EXPERIMENT_STATISTICS_H
#define ISPP_EXPERIMENT_STATISTICS_H

#include <vector>

namespace ispp {

/// 一组样本的统计聚合（蒙特卡洛次数 > 1 时使用）。
struct MetricStats {
    double Mean;
    double Std;
    double Min;
    double Max;
};

/// 对 samples 计算算术均值 / 样本标准差 / 极小 / 极大。
/// 空序列返回全零 MetricStats。
MetricStats computeStats(const std::vector<double> &samples);

} // namespace ispp

#endif // ISPP_EXPERIMENT_STATISTICS_H
