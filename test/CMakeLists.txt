cmake_minimum_required(VERSION 3.10)

add_executable(test_fft test_fft.cpp)
target_link_libraries(test_fft PRIVATE PocketFFT)
add_test(NAME test_fft COMMAND test_fft)