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

Make sure the MSYS2 executables are discoverable in the Windows PowerShell environment.

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

To update the internationalization translation files, run the following commands in PowerShell:

```pwsh
xgettext -C --keyword=_UI --from-code=UTF-8 -o .\locales\pot\ui.pot @((Get-ChildItem -Path "src" -Recurse -Filter "*.cpp").FullName)
msginit -l zh_CN.UTF-8 -i .\locales\pot\ui.pot -o .\locales\zh_CN\ui.po
```

> **Note**: do **not** delete the `locales/zh_CN/ui.po` file shipped with the repository — it contains essential translation entries that `xgettext` cannot auto-generate. The command above can be used to update the existing entries.

After configuring with CMake, CMake's build system will automatically compile the `.po` file into a `.mo` binary and place it in the `build` directory. Then build with:

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
