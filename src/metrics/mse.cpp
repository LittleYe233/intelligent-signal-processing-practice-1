#include "ispp/metrics/mse.h"
#include "ispp/i18n.h"
#include <algorithm>
#include <cmath>
#include <format>

namespace ispp {

double MseMetric::evaluate(double true_frequency_hz,
                           const EstimationResult &result) {
    if (result.Peaks.empty()) {
        return 0.0;
    }

    // OQ-21: select the peak with the highest Prominence (not min-error),
    // because in real estimation we don't know the true frequency.
    const auto BEST = std::ranges::max_element(
        result.Peaks, std::less<>{},
        [](const FrequencyPeak &p) { return p.Prominence; });

    const double ERROR = BEST->FrequencyHz - true_frequency_hz;
    return ERROR * ERROR;
}

std::string_view MseMetric::name() const { return _UI("MSE"); }

std::string MseMetric::format(double value) const {
    return std::format("{:.6e}", value);
}

} // namespace ispp
