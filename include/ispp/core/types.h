#ifndef ISPP_CORE_TYPES_H
#define ISPP_CORE_TYPES_H

#include <complex>
#include <vector>

namespace ispp {

using RealArray = std::vector<double>;
using ComplexArray = std::vector<std::complex<double>>;

/// Denotes that amplitude of such frequency is unknown. Never assume a specific
/// value and should be handled properly in other procedures.
const double AMP_UNKNOWN = -1;

struct FrequencyPeak {
    double FrequencyHz;
    double Amplitude;
};

struct EstimationResult {
    std::vector<FrequencyPeak> Peaks;
    double ComputeTimeSec;
};

} // namespace ispp

#endif // ISPP_CORE_TYPES_H
