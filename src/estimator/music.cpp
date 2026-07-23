#include "ispp/estimator/music.h"
#include "ispp/core/fft.h"
#include "ispp/core/types.h"
#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <functional>
#include <vector>

namespace {

// Little helper struct
struct Peak {
    double Freq;
    double Power;
};

std::vector<ispp::FrequencyPeak>
// NOLINTNEXTLINE(readability-identifier-naming)
musicCalc(const ispp::RealArray &sig, const int L, const int p, const double F,
          const double deltaF, const double minF, const double maxF) {
    // Step 1: Determine parameters
    // Real frequency search span, taking deltaF into consideration
    const double MIN_F = minF - deltaF, MAX_F = maxF + deltaF;
    // Cond: d <= F / (2 * f_max)
    int d = std::max(1, (int)floor(F / 2.0 / MAX_F));
    // Determine M
    // NOLINTNEXTLINE(readability-identifier-naming)
    int M = std::min(L / (2 * d), 64);
    if (M <= p) {
        M = p + 2;
    }
    // Cond: L >= (M-1)d + N
    // NOLINTNEXTLINE(readability-identifier-naming)
    int N = L - (M - 1) * d;
    // Determine m0 and B
    int m0 = std::max(0, (int)floor(MIN_F * M * d / F));
    // NOLINTNEXTLINE(readability-identifier-naming)
    int B = (int)ceil(MAX_F * M * d / F) - m0 + 1;
    // Safety check
    B = std::max(B, p + 1);
    B = std::min(B, M); // B < M/2 may be better?

    // Step 2: Construct snapshot matrix
    // NOLINTNEXTLINE(readability-identifier-naming)
    Eigen::MatrixXcd Y(M, N);
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            Y(m, n) = std::complex<double>(
                sig[(size_t)n + (size_t)m * (size_t)d], 0.0);
        }
    }

    // Step 3: Construct DFT beam-form motrix
    // NOLINTNEXTLINE(readability-identifier-naming)
    Eigen::MatrixXcd T(M, B);
    const std::complex<double> J(0.0, 1.0);
    for (int m = 0; m < M; ++m) {
        for (int b = 0; b < B; ++b) {
            int u = m0 + b;
            // T(u) = [1, e^{j2πu(1/M)}, ...]^T
            T(m, b) = (1.0 / sqrt(M)) * exp(J * 2.0 * std::numbers::pi *
                                            (double)(u * m) / (double)M);
        }
    }

    // Step 4: Construct beam-space co-variant matrix
    // 投影到频域波束空间: Yb = T^H * Y
    // NOLINTNEXTLINE(readability-identifier-naming)
    Eigen::MatrixXcd Yb = T.adjoint() * Y;
    // 协方差估计: RYb = (1/N) * Yb * Yb^H
    // NOLINTNEXTLINE(readability-identifier-naming)
    Eigen::MatrixXcd RYb = (Yb * Yb.adjoint()) / (double)N;

    // Step 5: Decompose eigenvalues, extract noise subspace
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXcd> solver(RYb);
    // NOLINTNEXTLINE(readability-identifier-naming)
    Eigen::MatrixXcd Gb = solver.eigenvectors().leftCols(B - p);

    // Step 6: Search peaks in 1-D frequency search span
    const double SEARCH_F_START = m0 * F / (M * d);
    const double SEARCH_F_END = (B - 1 + m0) * F / (M * d);
    const double STEP_SIZE = deltaF / 50.0; // 细化网格精度
    ispp::RealArray scan_freqs, scan_spectrum;
    // NOLINTNEXTLINE(readability-identifier-naming)
    Eigen::MatrixXcd GbGbH = Gb * Gb.adjoint();
    for (double f = SEARCH_F_START; f <= SEARCH_F_END; f += STEP_SIZE) {
        // 构建频率导向矢量
        Eigen::VectorXcd a(M);
        for (int m = 0; m < M; ++m) {
            a(m) = exp(J * 2.0 * std::numbers::pi * f * (double)(m * d) / F);
        }
        // 谱函数分母计算 D_FB_MUSIC(f) = a^H * T * Gb * Gb^H * T^H * a
        // NOLINTNEXTLINE(readability-identifier-naming)
        Eigen::VectorXcd THa = T.adjoint() * a;
        // NOLINTNEXTLINE(readability-identifier-naming)
        std::complex<double> D_val = THa.adjoint() * GbGbH * THa;
        // 谱函数
        double p_music = 10.0 * log10(1.0 / std::abs(D_val.real()));
        scan_freqs.push_back(f);
        scan_spectrum.push_back(p_music);
    }
    // Find local maximums
    std::vector<Peak> local_peaks;
    for (size_t i = 1; i < scan_spectrum.size() - 1; ++i) {
        if (scan_spectrum[i] > scan_spectrum[i - 1] &&
            scan_spectrum[i] > scan_spectrum[i + 1]) {
            local_peaks.push_back(
                {.Freq = scan_freqs[i], .Power = scan_spectrum[i]});
        }
    }
    // Sort and select the biggest p ones
    std::ranges::sort(local_peaks, std::ranges::greater{}, &Peak::Power);
    std::vector<ispp::FrequencyPeak> final_peaks;
    for (size_t i = 0; i < std::min((size_t)p, local_peaks.size()); ++i) {
        final_peaks.push_back(
            ispp::FrequencyPeak{.FrequencyHz = local_peaks[i].Freq,
                                .Amplitude = ispp::AMP_UNKNOWN,
                                .Prominence = ispp::PROMINENCE_UNKNOWN});
    }
    std::ranges::sort(final_peaks, std::ranges::less{},
                      &ispp::FrequencyPeak::FrequencyHz);
    return final_peaks;
}

} // namespace

namespace ispp {

MusicEstimator::MusicEstimator(double threshold) : Threshold(threshold) {}

std::vector<FrequencyPeak>
MusicEstimator::estimate(const RealArray &input,
                         const EstimationContext &context) {
    if (input.empty()) {
        return {};
    }

    const ComplexArray DFT = computeDft(input);
    if (DFT.empty()) {
        return {};
    }

    const size_t N = input.size();
    const double BIN_HZ = context.SampleRateHz / static_cast<double>(N);
    auto peaks =
        findPeaksFromDft(DFT, Threshold, BIN_HZ, context.FrequencyCount);
    if (peaks.empty()) {
        return {};
    }
    const auto [MIN_PEAK, MAX_PEAK] =
        std::ranges::minmax(peaks, {}, &FrequencyPeak::FrequencyHz);

    auto final_peaks =
        musicCalc(input, (int)N, (int)peaks.size(), context.SampleRateHz,
                  BIN_HZ, MIN_PEAK.FrequencyHz, MAX_PEAK.FrequencyHz);

    return final_peaks;
}

std::string_view MusicEstimator::name() const { return "MUSIC"; }

} // namespace ispp
