# Harbin Institute of Technology (Shenzhen) — *Intelligent Signal Processing Practice*, Assignment 1 Source Code

This project provides a simulation and testing platform for frequency-estimation algorithms of real-valued sinusoidal signals under noise and interference, and also supports batch parameter-scan tests.

## Build

This project is designed for Windows, and both development and builds are done on Windows. It assumes an MSYS2 environment with a UCRT64 shell.

First, install the toolchain and environment packages:

```shell
pacman -Syu mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-gdb mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-clang-tools-extra mingw-w64-ucrt-x86_64-cmake
```

Then install the required third-party libraries:

```shell
pacman -Syu mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-gettext-runtime mingw-w64-ucrt-x86_64-gettext-tools
```

Clone this repository:

```shell
git clone https://github.com/LittleYe233/intelligent-signal-processing-practice-1.git
```

This repository includes other repositories as submodules; sync them with:

```shell
git submodule update --init --recursive
```

CMake is configured with multiple build variants:

```shell
# Typical development: use the Debug variant
cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -B build -G Ninja

# Production build: use the Release variant and enable best optimizations for the current CPU
cmake -DISPP_ENABLE_NATIVE=ON -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -B build -G Ninja
```

After configuring with CMake, build with:

```shell
cmake --build build
```

## Tech Stack

- Language and build system: C++20 & CMake
- GUI and plotting: ImGui & ImPlot
- Graphics backend: OpenGL3 & GLFW
- Linear algebra: Eigen
- Lightweight FFT: PocketFFT
- Internationalization: GNU Gettext

## License

[MIT License](/LICENSE)
