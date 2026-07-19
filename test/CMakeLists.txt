cmake_minimum_required(VERSION 3.10)

# ---------------------------------------------------------------------------
# test_fft
# ---------------------------------------------------------------------------

add_executable(test_fft test_fft.cpp)
target_link_libraries(test_fft PRIVATE PocketFFT)
add_test(NAME test_fft COMMAND test_fft)

# ---------------------------------------------------------------------------
# test_implot
# ---------------------------------------------------------------------------

add_executable(test_implot test_implot.cpp)
target_link_libraries(test_implot PRIVATE implot)
add_test(NAME test_implot COMMAND test_implot)