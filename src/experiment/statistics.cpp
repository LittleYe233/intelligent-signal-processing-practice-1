#include "ispp/experiment/statistics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace ispp {

MetricStats computeStats(const std::vector<double> &samples) {
    if (samples.empty()) {
        return MetricStats{};
    }

    // Sum
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);

    double mean = sum / static_cast<double>(samples.size());

    // Variance (sample std: 1/(N-1))
    double sq_sum = 0.0;
    for (const auto &x : samples) {
        double diff = x - mean;
        sq_sum += diff * diff;
    }
    double variance = (samples.size() > 1)
                          ? sq_sum / static_cast<double>(samples.size() - 1)
                          : 0.0;
    double std = std::sqrt(variance);

    // Min / Max
    auto [min_it, max_it] = std::ranges::minmax_element(samples);

    return MetricStats{
        .Mean = mean,
        .Std = std,
        .Min = (min_it != samples.end()) ? *min_it : 0.0,
        .Max = (max_it != samples.end()) ? *max_it : 0.0,
    };
}

} // namespace ispp
