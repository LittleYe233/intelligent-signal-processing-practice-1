#include "ispp/metrics/rmse.h"
#include "ispp/i18n.h"
#include <algorithm>
#include <cmath>
#include <format>

namespace ispp {

double RmseMetric::evaluate(double true_frequency_hz,
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

    const double ERROR = BEST->FrequencyHz - true_frequency_hz;
    return ERROR * ERROR;
}

std::string_view RmseMetric::name() const { return _UI("RMSE"); }

std::string RmseMetric::format(double value) const {
    return std::format("{:.6e}", std::sqrt(value));
}

} // namespace ispp
