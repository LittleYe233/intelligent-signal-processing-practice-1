#ifndef ISPP_ESTIMATOR_ESPRIT_H
#define ISPP_ESTIMATOR_ESPRIT_H

#include "ispp/estimator/estimator.h"
#include <string_view>
#include <vector>

namespace ispp {

/// ESPRIT 频率估计算法 — Standard ESPRIT with forward-backward averaging.
///
/// Uses FB-averaged Hankel matrix + real covariance EVD + standard ESPRIT
/// selection to estimate frequencies from the shift-invariance property.
/// All arithmetic is real-valued except the final r×r complex EVD.
///
/// Adaptive window length L (§4 of esprit.md):
///   K == 1, N ≥ 128 → L = N/4  (aggressive, ~8× speedup)
///   K == 2         → L = N/2  (conservative)
///   N ≤ 64         → L = N/2
///
/// Ignores context.WindowKind — ESPRIT requires raw unwindowed data per
/// the Vandermonde model.
///
/// @see .opencode/context/esprit.md (v2.0 analysis)
/// @see H95: 10.1109/78.382406  (Unitary ESPRIT)
/// @see D24: 10.1109/FOCS61266.2024.00137
/// @note Requires -O3 for Eigen SIMD vectorization (enabled in CMakeLists.txt).
class EspritEstimator final : public IEstimator {
public:
    EspritEstimator() = default;

    /// @see .opencode/context/esprit.md
    /// @note Ignores context.WindowKind — ESPRIT uses raw unwindowed data.
    std::vector<FrequencyPeak>
    estimate(const RealArray &input, const EstimationContext &context) override;

    std::string_view name() const override;
};

} // namespace ispp

#endif // ISPP_ESTIMATOR_ESPRIT_H
