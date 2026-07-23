#include "ispp/estimator/esprit.h"
#include "Eigen/Core"
#include "ispp/core/types.h"
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <Eigen/SVD>
#include <complex>
#include <vector>

namespace {

// NOLINTBEGIN(readability-identifier-naming)
Eigen::MatrixXcd solve_pseudo_inverse(const Eigen::MatrixXcd &A,
                                      const Eigen::MatrixXcd &B) {
    return A.bdcSvd<Eigen::ComputeThinU | Eigen::ComputeThinV>().solve(B);
}

std::vector<ispp::FrequencyPeak> espritCalc(const ispp::RealArray &sig,
                                            const int r, const double fs) {
    const auto N = (Eigen::Index)sig.size();

    // Step 1: Construct standard Hankel data matrix
    // Spatial smooth, set up sliding window to decrease computation complexity
    const Eigen::Index L = N / 2, M = N - L + 1;
    Eigen::MatrixXcd X(L, M);
    for (int i = 0; i < L; ++i) {
        for (int j = 0; j < M; ++j) {
            X(i, j) = sig[(size_t)i + (size_t)j];
        }
    }
    // Co-variant matrix
    Eigen::MatrixXcd R = (X * X.adjoint()) / static_cast<double>(M);

    // Step 2: Extract signal subspace (based on SVD necessarily)
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> eig_solver(R);
    Eigen::MatrixXcd Q_r = eig_solver.eigenvectors().rightCols(r);

    // Step 3: Construct rotation shift matrix
    Eigen::MatrixXcd Q_up = Q_r.topRows(L - 1);
    Eigen::MatrixXcd Q_down = Q_r.bottomRows(L - 1);

    // Step 4: Calculate pseudo inverse matrix
    Eigen::MatrixXcd W = solve_pseudo_inverse(Q_up, Q_down);

    // Step 5: Decompose eigen values
    Eigen::ComplexEigenSolver<Eigen::MatrixXcd> solver(W);
    Eigen::VectorXcd lambda = solver.eigenvalues();

    // Step 6: Extract freqs and construct Vandermonde matrix
    ispp::RealArray freqs((size_t)r);
    ispp::ComplexArray z((size_t)r);
    Eigen::MatrixXcd V(N, r);
    for (Eigen::Index k = 0; k < r; ++k) {
        // Map arg to [0,2pi)
        double angle = std::arg(lambda(k));
        if (angle < 0) {
            angle += 2 * std::numbers::pi;
        }
        // Map arg to real freq (WHAT WE WANT!)
        freqs[(size_t)k] = angle / (2 * std::numbers::pi) * fs;
        // Construct complex exponential roots on unit circle
        z[(size_t)k] = std::polar(1.0, angle);
        // Fill Vandermonde matrix
        for (Eigen::Index i = 0; i < N; ++i) {
            V(i, k) = std::pow(z[(size_t)k], i);
        }
    }

    // Step 7: Filter real freqs
    std::vector<ispp::FrequencyPeak> peaks;
    for (Eigen::Index k = 0; k < r; ++k) {
        // Only use positive freqs
        if (freqs[(size_t)k] <= fs / 2.0) {
            peaks.push_back(
                ispp::FrequencyPeak{.FrequencyHz = freqs[(size_t)k],
                                    .Amplitude = ispp::AMP_UNKNOWN,
                                    .Prominence = ispp::PROMINENCE_UNKNOWN});
        }
    }

    return peaks;
}
// NOLINTEND(readability-identifier-naming)

} // namespace

namespace ispp {

std::vector<FrequencyPeak>
EspritEstimator::estimate(const RealArray &input,
                          const EstimationContext &context) {
    // Complex signal frequency count (real * 2), a.k.a. subspace dimension
    const auto FREQ_CNT = context.FrequencyCount * 2;
    auto peaks = espritCalc(input, (int)FREQ_CNT, context.SampleRateHz);

    return peaks;
}

std::string_view EspritEstimator::name() const { return "ESPRIT"; }

} // namespace ispp
