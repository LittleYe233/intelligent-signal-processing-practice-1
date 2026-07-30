#include "ispp/metrics/percentage_error.h"
#include <algorithm>
#include <cmath>
#include <format>

namespace ispp {

double PercentageErrorMetric::evaluate(double true_frequency_hz,
                                       const EstimationResult &result) {
    if (result.Peaks.empty()) {
        return 0.0;
    }

    // Per OQ-6: compare with the peak that has minimum frequency error
    const auto BEST = std::ranges::min_element(
        result.Peaks, std::less<>{},
        [true_frequency_hz](const FrequencyPeak &p) {
            return std::abs(p.FrequencyHz - true_frequency_hz);
        });

    const double ERROR = std::abs(BEST->FrequencyHz - true_frequency_hz);
    return (ERROR / true_frequency_hz) * 100.0;
}

std::string_view PercentageErrorMetric::name() const {
    return "Percentage Error";
}

std::string PercentageErrorMetric::format(double value) const {
    return std::format("{:.4f}%", value);
}

} // namespace ispp
