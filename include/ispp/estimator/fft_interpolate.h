#ifndef ISPP_ESTIMATOR_FFT_INTERPOLATE_H
#define ISPP_ESTIMATOR_FFT_INTERPOLATE_H

#include "ispp/estimator/estimator.h"

#include <string_view>
#include <vector>

namespace ispp {

/// 基于 FFT 插值（抛物线 / Quinn）的频率估计算法。
/// 在 FFT 幅度谱峰值附近进行插值以突破 bin 分辨率限制。
/// 不同窗函数可能需要不同的插值系数，由 context.windowKind 选择。
class FftInterpolateEstimator final : public IEstimator {
public:
    explicit FftInterpolateEstimator(double threshold = 0.0);

    /// @todo 实现 FFT 插值频率估计
    ///   1. 使用 computeDft() 得到单边复数谱
    ///   2. 使用 findPeaksFromDft() 得到粗略峰值
    ///   3. 按 context.windowKind 选择插值算法（抛物线 / Quinn 等）
    ///   4. 对未知/不支持的窗回退到通用插值
    ///   5. 返回插值修正后的频率-幅度对列表
    std::vector<FrequencyPeak>
    estimate(const RealArray &input, const EstimationContext &context) override;

    std::string_view name() const override;

private:
    double Threshold;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_FFT_INTERPOLATE_H
