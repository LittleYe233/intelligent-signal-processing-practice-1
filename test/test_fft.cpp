#define _USE_MATH_DEFINES

#include <complex>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <pocketfft_hdronly.h>
#include <vector>

using namespace std;

int main() {
    // Sampling frequency = 8000 Hz
    const double fs = 8000;
    // Signal frequency = 1000 Hz
    const double f_sig = 1000;
    // Number of samples
    const size_t N = 8;

    // Signal: x[n]=3*sin(2*pi*f_sig*n/fs)
    vector<double> data_in(N);
    for (size_t n = 0; n < N; ++n) {
        data_in[n] = 3 * sin(2.0 * M_PI * f_sig * static_cast<double>(n) / fs);
    }

    size_t output_size = N / 2 + 1;
    vector<complex<double>> data_out(output_size);

    // Set up FFT axis configurations
    pocketfft::shape_t shape_in = { N };
    pocketfft::stride_t stride_in = { sizeof(double) };
    pocketfft::stride_t stride_out = { sizeof(complex<double>) };

    pocketfft::r2c(
        shape_in,
        stride_in,
        stride_out,
        (size_t)0,
        pocketfft::FORWARD,
        data_in.data(),
        data_out.data(),
        2.0 / N // normalization factor
    );

    // Output
    cout << "--- Input Signal x[n] ---\n";
    for (size_t n = 0; n < N; ++n) {
        std::cout << "x[" << n << "]: " << data_in[n] << "\n";
    }

    std::cout << "\n--- FFT Output (Frequency Domain) ---\n";
    for (size_t k = 0; k < output_size; ++k) {
        std::cout << "X[" << k << "]: " << data_out[k] << "\n";
    }

    return 0;
}