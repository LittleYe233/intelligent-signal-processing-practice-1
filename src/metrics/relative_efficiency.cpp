// DISABLED METRIC (OQ-20) — see relative_efficiency.h for re-enablement steps.
// This translation unit is compiled to detect API drift but is never linked
// into the active metric set at runtime.

#include "ispp/metrics/relative_efficiency.h"
#include "ispp/i18n.h"
#include <cmath>
#include <format>
#include <limits>
#include <numbers>
#include <numeric>
#include <vector>

namespace ispp {

double RelativeEfficiencyMetric::evaluate(double true_frequency_hz,
                                          const EstimationResult &result) {
    (void)true_frequency_hz;
    (void)result;
    return 0.0; // Aggregate metric — Runner calls finalize() instead
}

std::string_view RelativeEfficiencyMetric::name() const {
    return "Relative Efficiency";
}

std::string RelativeEfficiencyMetric::format(double value) const {
    if (std::isnan(value)) {
        return _UI("N/A");
    }
    return std::format("{:.6f}", value);
}

double RelativeEfficiencyMetric::finalize(
    const std::vector<double> &freq_estimates, const double sampleRateHz,
    const std::size_t sampleCount, const NoiseInfo noiseInfo) const {

    const std::size_t M = freq_estimates.size();
    if (M < 2) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Uniform / Impulse: CRB regularity conditions not met
    if (noiseInfo.Distribution == NoiseDistribution::UNIFORM ||
        noiseInfo.Distribution == NoiseDistribution::IMPULSE) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Sample variance of frequency estimates
    // f_avg = 1/M * Σ f_i ; sample_var = 1/(M-1) * Σ (f_i - f_avg)²
    const double SUM =
        std::accumulate(freq_estimates.begin(), freq_estimates.end(), 0.0);
    const double F_AVG = SUM / static_cast<double>(M);

    double sq_sum = 0.0;
    for (const double F : freq_estimates) {
        const double DIFF = F - F_AVG;
        sq_sum += DIFF * DIFF;
    }
    const double SAMPLE_VAR = sq_sum / static_cast<double>(M - 1);

    if (SAMPLE_VAR == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Noise std from SNR (signal amplitude = 1.0 → signal power = 0.5)
    const double SNR_DB = noiseInfo.SnrDb;
    const double NOISE_POWER = 0.5 / std::pow(10.0, SNR_DB / 10.0);

    // CRB
    const double FS = sampleRateHz;
    const auto N = static_cast<double>(sampleCount);
    const double DENOM =
        std::numbers::pi * std::numbers::pi * N * (N * N - 1.0);

    double crb;
    switch (noiseInfo.Distribution) {
    case NoiseDistribution::GAUSSIAN:
        crb = 6.0 * FS * FS * NOISE_POWER / DENOM;
        break;
    case NoiseDistribution::LAPLACIAN:
        crb = 3.0 * FS * FS * NOISE_POWER / DENOM;
        break;
    default:
        return std::numeric_limits<double>::quiet_NaN();
    }

    return crb / SAMPLE_VAR;
}

} // namespace ispp
