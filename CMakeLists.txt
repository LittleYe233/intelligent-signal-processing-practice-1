cmake_minimum_required(VERSION 3.10)
project(ISPPracticeOne VERSION 1.0.0 LANGUAGES CXX)

# Enforce strict C++20 standards
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type to release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Build options
option(ISPP_ENABLE_NATIVE OFF)
option(ISPP_ENABLE_X86_64_V4 OFF)
option(ISPP_ENABLE_X86_64_V3 OFF)
option(ISPP_WIN32_GUI "Build as windowed GUI app (no console)" OFF)

# MSYS2/GCC 16 warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-g)
    endif()
endif()

# Set build type options
if(CMAKE_BUILD_TYPE STREQUAL Release)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O3 -ffunction-sections -fdata-sections)
        add_link_options(-Wl,-s -flto -Wl,--gc-sections)
    endif()
endif()

# Set optimization options
if(ISPP_ENABLE_NATIVE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-march=native)
    endif()
elseif(ISPP_ENABLE_X86_64_V4)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-march=x86-64-v4 -mtune=x86-64-v4)
    endif()
elseif(ISPP_ENABLE_X86_64_V3)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-march=x86-64-v3 -mtune=x86-64-v3)
    endif()
endif()

# Windows specific flag: Hides the console window for GUI mode
if(ISPP_WIN32_GUI)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# ---------------------------------------------------------------------------
# Third-Party Dependencies (Git Submodules)
# ---------------------------------------------------------------------------

find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

set(imgui_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCE})
target_include_directories(imgui PUBLIC "${imgui_SOURCE_DIR};${imgui_SOURCE_DIR}/backends/")
target_link_libraries(imgui PUBLIC glfw3 OpenGL::GL)

set(implot_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot)
set(IMPLOT_SOURCE
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)
add_library(implot STATIC ${IMPLOT_SOURCE})
target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

add_library(PocketFFT INTERFACE)
target_include_directories(PocketFFT INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pocketfft)

add_library(Eigen INTERFACE)
target_include_directories(Eigen INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/eigen/Eigen)

# ---------------------------------------------------------------------------
# Testing
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(test)

# ---------------------------------------------------------------------------
# Main Application Target
# ---------------------------------------------------------------------------

set(ISPP_SOURCES
    src/main.cpp
    # Core
    src/core/rng.cpp
    src/core/fft.cpp
    # Window
    src/window/window.cpp
    # Signal
    src/signal/signal_generator.cpp
    # Estimator (M2)
    src/estimator/fft_peak.cpp
    src/estimator/fft_interpolate.cpp
    src/estimator/music.cpp
    src/estimator/esprit.cpp
    # Metrics
    src/metrics/percentage_error.cpp
    src/metrics/rmse.cpp
    src/metrics/compute_time.cpp
    # Experiment (M1)
    src/experiment/statistics.cpp
    src/experiment/experiment_runner.cpp
    src/ui/ui_manager.cpp
    src/ui/panels/config_panel.cpp
    src/ui/panels/spectrum_panel.cpp
    src/ui/panels/results_panel.cpp
    src/ui/panels/log_panel.cpp
)

add_executable(${PROJECT_NAME} ${ISPP_SOURCES})

# Setup include directories for our own code
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# # Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE glfw3 OpenGL::GL imgui implot PocketFFT Eigen)