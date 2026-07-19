#define _USE_MATH_DEFINES

#include <cmath>
#include <complex>
#include <cstddef>
#include <iostream>
#include <pocketfft_hdronly.h>
#include <vector>

using namespace std;

/// Calculates FFT spectrum resolution.
constexpr double FFT_REZ(double fs, size_t n) {
    return fs / static_cast<double>(n);
}

/// Calculates frequency (Hz) of one specific slot of FFT spectrum.
constexpr double FFT_FREQ(double rez, size_t k) {
    return rez * static_cast<double>(k);
}

// len: length of dft array
// dft: should be normalized first
template <typename T>
static size_t estimateFreqs(const size_t len, const complex<T> *dft,
                            size_t *indices, double *amps,
                            const double thresholdFactor) {
    vector<T> real_dft(len);
    T max_real_dft = -1;
    // Pick modulus and get maximum
    for (size_t n = 0; n < len; ++n) {
        real_dft[n] = abs(dft[n]);
        if (real_dft[n] > max_real_dft) {
            max_real_dft = real_dft[n];
        }
    }
    T threshold = max_real_dft * thresholdFactor;
    // Find peaks
    size_t n_peaks = 0;
    for (size_t n = 1; n < len - 1; ++n) {
        if (real_dft[n] > real_dft[n - 1] && real_dft[n] > real_dft[n + 1]) {
            T left = real_dft[n] - real_dft[n - 1],
              right = real_dft[n] - real_dft[n + 1];
            if (left >= threshold && right >= threshold) {
                indices[n_peaks] = n;
                amps[n_peaks] = real_dft[n];
                ++n_peaks;
            }
        }
    }
    return n_peaks;
}

int main() {
    // Sampling frequency = 8000 Hz
    const double FS = 8000;
    // Signal frequency = 1250 Hz (should cause spectrum leakage)
    const double F_SIG = 1250;
    // Number of samples
    const size_t N = 16;
    // FFT resolution
    const double REZ = FFT_REZ(FS, N);

    // Signal: x[n]=3*sin(2*pi*f_sig*n/fs)
    vector<double> data_in(N);
    for (size_t n = 0; n < N; ++n) {
        data_in[n] = 3 * sin(2.0 * M_PI * F_SIG * static_cast<double>(n) / FS);
    }

    size_t output_size = N / 2 + 1;
    vector<complex<double>> data_out(output_size);

    // Set up FFT axis configurations
    pocketfft::shape_t shape_in = {N};
    pocketfft::stride_t stride_in = {sizeof(double)};
    pocketfft::stride_t stride_out = {sizeof(complex<double>)};

    try {
        pocketfft::r2c(shape_in, stride_in, stride_out, 0, pocketfft::FORWARD,
                       data_in.data(), data_out.data(),
                       2.0 / N // normalization factor
        );
    } catch (const exception &e) {
        std::cerr << "Fatal error when calculating FFT: " << e.what() << '\n';
        return 1;
    }

    // Output
    cout << "--- Input Signal x[n] ---\n";
    for (size_t n = 0; n < N; ++n) {
        std::cout << "x[" << n << "]: " << data_in[n] << "\n";
    }

    std::cout << "\n--- FFT Output (Frequency Domain) ---\n";
    for (size_t k = 0; k < output_size; ++k) {
        std::cout << "X[" << k << "]: " << data_out[k]
                  << " abs=" << abs(data_out[k]) << " freq=" << FFT_FREQ(REZ, k)
                  << "\n";
    }

    std::cout << "\n--- Find Peaks ---\n";
    size_t max_possible_len = N / 4;
    vector<double> amps(max_possible_len);
    vector<size_t> indices(max_possible_len);
    size_t n_peaks = estimateFreqs(data_out.size(), data_out.data(),
                                   indices.data(), amps.data(), 0.05);
    for (size_t i = 0; i < n_peaks; ++i) {
        cout << "Peaks[" << i << "]: " << FFT_FREQ(REZ, indices[i])
             << " amp=" << amps[i] << "\n";
    }

    return 0;
}