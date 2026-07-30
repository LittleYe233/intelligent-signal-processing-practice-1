#include "ispp/estimator/esprit.h"
#include "Eigen/Core"
#include "ispp/core/types.h"
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace {

// NOLINTBEGIN(readability-identifier-naming)

/// Adaptive Hankel window length.
///
/// K == 2: L = N/2 (conservative — must handle unknown min-frequency-spacing)
/// K == 1: L = N/4 for N ≥ 128 (aggressive — ~8× speedup; MC averaging
///         preserves accuracy per Ding Theorem I.4)
/// N ≤ 64:  L = N/2 (too small to safely reduce)
Eigen::Index adaptWindowLength(Eigen::Index N, Eigen::Index K) {
    if (N <= 64)
        return N / 2;
    if (K == 2)
        return N / 2;
    // K == 1, N >= 128
    return std::max(K * 2 + 1, N / 4);
}

// ---------------------------------------------------------------------------
// Standard ESPRIT with forward-backward averaging (all-real except final EVD)
//
// Replaces the Unitary ESPRIT Q-transform + K₁/K₂ selection with standard
// ESPRIT selection matrices on the FB-averaged real covariance eigenvectors.
// Mathematically sound — the FB-averaged Hankel matrix preserves the shift-
// invariance property in its real-valued signal subspace.
// ---------------------------------------------------------------------------
std::vector<ispp::FrequencyPeak> espritCalc(const double *sig, Eigen::Index N,
                                            Eigen::Index L, Eigen::Index M,
                                            Eigen::Index r, double fs) {
    (void)N;

    // === Step 1: Hankel data matrix ===
    Eigen::MatrixXd X(L, M);
    for (Eigen::Index i = 0; i < L; ++i)
        for (Eigen::Index j = 0; j < M; ++j)
            X(i, j) = sig[i + j];

    // === Step 2: Forward-backward averaging ===
    // Z = [X, Π_L·X·Π_M]  →  doubles effective snapshots (M → 2M)
    Eigen::MatrixXd Z(L, 2 * M);
    Z.leftCols(M) = X;
    Z.rightCols(M) = X.colwise().reverse().rowwise().reverse();

    // === Step 3: Real covariance matrix ===
    Eigen::MatrixXd R =
        Z * Z.transpose(); // omit 1/(2M) — eigenvectors unchanged

    // === Step 4: Signal subspace via real symmetric EVD ===
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig_solver(R);
    if (eig_solver.info() != Eigen::Success)
        return {};
    Eigen::MatrixXd E_s = eig_solver.eigenvectors().rightCols(r);

    // === Step 5: Standard ESPRIT selection matrices (no Q-transform) ===
    // The shift-invariance property holds for the FB-averaged real
    // eigenvectors.
    Eigen::MatrixXd U1 = E_s.topRows(L - 1);    // rows 0 .. L-2
    Eigen::MatrixXd U2 = E_s.bottomRows(L - 1); // rows 1 .. L-1

    // === Step 6: LS solution via normal equations ===
    // Solve U₁ · Ψ = U₂  for the r×r rotation matrix Ψ.
    Eigen::MatrixXd AtA = U1.transpose() * U1;
    Eigen::MatrixXd AtB = U1.transpose() * U2;
    Eigen::MatrixXd Psi = AtA.ldlt().solve(AtB);

    // === Step 7: Complex EVD of Ψ (r×r, tiny) ===
    // Eigenvalues λ_k = e^{j·μ_k} encode the digital frequencies.
    Eigen::ComplexEigenSolver<Eigen::MatrixXd> psi_solver(Psi);
    if (psi_solver.info() != Eigen::Success)
        return {};
    Eigen::VectorXcd lambda = psi_solver.eigenvalues();

    // === Step 8: Frequency extraction ===
    //   μ_k = arg(λ_k) ∈ (−π, π]
    //   f̂_k = f_s · |μ_k| / (2π)
    // Reject λ_k far from the unit circle (noise-driven modes).
    std::vector<double> freqs;
    freqs.reserve(static_cast<std::size_t>(r));
    double nyquist = fs / 2.0;
    constexpr double UNIT_CIRCLE_TOL = 1e-1;

    for (Eigen::Index i = 0; i < r; ++i) {
        std::complex<double> l = lambda(i);

        // Unit-circle check: |λ| should be close to 1 for valid signal roots
        double mag = std::abs(l);
        if (mag < 1.0 - UNIT_CIRCLE_TOL || mag > 1.0 + UNIT_CIRCLE_TOL)
            continue;

        double angle = std::arg(l);
        if (angle < 0.0)
            angle += 2.0 * std::numbers::pi;
        double f_est = fs * angle / (2.0 * std::numbers::pi);

        // Fold > Nyquist frequencies
        if (f_est > nyquist)
            f_est = fs - f_est;
        if (f_est > 0.0 && f_est < nyquist)
            freqs.push_back(f_est);
    }

    // Deduplicate: r = 2K eigenvalues come in conjugate pairs → K unique freqs
    std::ranges::sort(freqs);
    auto last = std::ranges::unique(
        freqs, [](double a, double b) { return std::abs(a - b) < 1e-6; });
    freqs.erase(last.begin(), freqs.end());

    // === Step 9: Output — all candidates, no internal frequency filtering ===
    std::vector<ispp::FrequencyPeak> peaks;
    peaks.reserve(freqs.size());
    for (double f : freqs)
        peaks.push_back(
            ispp::FrequencyPeak{.FrequencyHz = f,
                                .Amplitude = ispp::AMP_UNKNOWN,
                                .Prominence = ispp::PROMINENCE_UNKNOWN});
    return peaks;
}

// NOLINTEND(readability-identifier-naming)

} // namespace

// ===========================================================================
// Public interface
// ===========================================================================
namespace ispp {

std::vector<FrequencyPeak>
EspritEstimator::estimate(const RealArray &input,
                          const EstimationContext &context) {
    const auto N = static_cast<Eigen::Index>(input.size());
    const auto K = static_cast<Eigen::Index>(context.FrequencyCount);
    const Eigen::Index R = 2 * K; // real signal → 2K complex exponentials
    const Eigen::Index L = adaptWindowLength(N, K);
    const Eigen::Index M = N - L + 1;

    return espritCalc(input.data(), N, L, M, R, context.SampleRateHz);
}

std::string_view EspritEstimator::name() const { return "ESPRIT"; }

} // namespace ispp
