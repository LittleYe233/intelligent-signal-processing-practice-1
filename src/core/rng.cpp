#include "ispp/core/rng.h"
#include <random>

namespace ispp {

Rng::Rng(std::uint64_t seed) : Engine(seed) {}

double Rng::normal(double mean, double stddev) {
    std::normal_distribution<double> dist(mean, stddev);
    return dist(Engine);
}

double Rng::uniform(double lo, double hi) {
    std::uniform_real_distribution<double> dist(lo, hi);
    return dist(Engine);
}

double Rng::laplace(double mean, double scale) {
    // Laplace(mean, scale) = mean + Exp(1/scale) - Exp(1/scale)
    std::exponential_distribution<double> exp_dist(1.0 / scale);
    return mean + exp_dist(Engine) - exp_dist(Engine);
}

double Rng::impulse(double p, double magnitude) {
    std::bernoulli_distribution dist(p);
    return dist(Engine) ? magnitude : 0.0;
}

} // namespace ispp
