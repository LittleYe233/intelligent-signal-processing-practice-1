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

/// Denotes that prominence of such frequency is unknown / meaningless.
/// Used by estimators that do not obtain peaks via PeakFinder (e.g. FFT
/// Interpolate produces a single interpolated frequency with no associated
/// prominence). See OQ-18.
const double PROMINENCE_UNKNOWN = -1;

struct FrequencyPeak {
    double FrequencyHz;
    double Amplitude;
    double Prominence;
};

struct EstimationResult {
    std::vector<FrequencyPeak> Peaks;
    double ComputeTimeSec;
};

} // namespace ispp

#endif // ISPP_CORE_TYPES_H
