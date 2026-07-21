#include "ispp/metrics/compute_time.h"

namespace ispp {

double ComputeTimeMetric::evaluate(double true_frequency_hz,
                                   const EstimationResult &result) {
    (void)true_frequency_hz;
    return result.ComputeTimeSec;
}

std::string_view ComputeTimeMetric::name() const { return "Compute Time"; }

} // namespace ispp
