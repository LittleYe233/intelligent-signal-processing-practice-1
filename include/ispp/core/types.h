#ifndef ISPP_CORE_TYPES_H
#define ISPP_CORE_TYPES_H

#include <string_view>
#include <vector>

namespace ispp {

using RealArray = std::vector<double>;

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
