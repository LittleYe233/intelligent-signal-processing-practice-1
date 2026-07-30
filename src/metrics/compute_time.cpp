#include "ispp/metrics/compute_time.h"
#include <format>
#include <string>

namespace ispp {

double ComputeTimeMetric::evaluate(double true_frequency_hz,
                                   const EstimationResult &result) {
    (void)true_frequency_hz;
    return result.ComputeTimeSec;
}

std::string_view ComputeTimeMetric::name() const { return "Compute Time"; }

std::string ComputeTimeMetric::format(double value) const {
    double scaled;
    const char *unit;

    if (value >= 1.0) {
        scaled = value;
        unit = "s";
    } else if (value >= 1e-3) {
        scaled = value * 1e3;
        unit = "ms";
    } else if (value >= 1e-6) {
        scaled = value * 1e6;
        unit = "us";
    } else {
        scaled = value * 1e9;
        unit = "ns";
    }

    if (scaled >= 100.0)
        return std::format("{:.0f}{}", scaled, unit);
    if (scaled >= 10.0)
        return std::format("{:.1f}{}", scaled, unit);
    return std::format("{:.2f}{}", scaled, unit);
}

} // namespace ispp
