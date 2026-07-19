# AGENTS.md

Compact orientation for AI agents working in this repo.

## Project

C++20 signal-processing practice app (`ISPPracticeOne`). `src/main.cpp` is
currently a stub; the real runnable targets live in `test/` and are
**interactive smoke-tests of third-party libraries** (ImGui/ImPlot/PocketFFT),
not unit tests. Treat them as exploratory harnesses, not as a test suite.

## Toolchain (Windows / MSYS2 only)

This project targets **MSYS2 UCRT64**. The build expects:
- `gcc`/`g++`, `cmake`, `ninja` from `C:\msys64\ucrt64\bin`
- `glfw3` installed system-wide via `pacman -S mingw-w64-ucrt-x86_64-glfw`
  (resolved via `find_package(glfw3)` — not vendored)
- OpenGL

A fresh checkout needs submodules:
```sh
git submodule update --init --recursive
```

## Build

Use this exact configure command — it matches the VS Code CMake Tools setup
and is the canonical way to build the project:
```sh
cmake -S . -B build -G Ninja -Wno-unused-cli \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Notes:
- `-Wno-unused-cli` suppresses CMake warnings from the (intentionally
  optional) `ISPP_ENABLE_*` flags.
- `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` is required — clangd reads
  `build/compile_commands.json`.
- The CMake default is `Release`, but **develop in `Debug`** to match the
  team's working configuration.

## Tests — do not run

`test/` currently holds interactive demo executables, not automated unit
tests. **For now, do not run `ctest`, do not add unit tests, and do not gate
work on test results.** Treat the existing `test_*.cpp` files as manual
exploration of the dependencies only.

## Build options (off by default)

Architecture-specific optimization is **not** enabled by default. Pass one of:
- `-DISPP_ENABLE_NATIVE=ON` (`-march=native`)
- `-DISPP_ENABLE_X86_64_V4=ON` (`-march=x86-64-v4`)
- `-DISPP_ENABLE_X86_64_V3=ON` (`-march=x86-64-v3`)

These are mutually exclusive (first match wins, see `CMakeLists.txt`).

## Code style (enforced, not advisory)

- **C++20 strict** — `CMAKE_CXX_EXTENSIONS=OFF`. Do not use GNU extensions.
- **Strict warnings on GCC/Clang**: `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`.
  `-Wconversion` in particular will reject many "normal-looking" assignments;
  use explicit casts (see `test/test_fft.cpp`).
- **clang-format** (`.clang-format`): LLVM base, 4-space indent.
- **clang-tidy** (`.clang-tidy`) naming is enforced:
  - `ConstantCase: UPPER_CASE`
  - `ClassCase / EnumCase / UnionCase / MemberCase: CamelCase`
  - `FunctionCase: camelBack`
  - `ParameterCase / VariableCase: lower_case`
- **clangd is the language server**, not IntelliSense.

### Before any further action after editing code

1. Run **clang-format** on every changed file (VS Code's `formatOnSave` covers
   this if you edit there).
2. Run **clang-tidy** and resolve every diagnostic it raises.

Do not commit, build, or move on until both are clean for the files you
touched.

## Dependency wiring

All in `CMakeLists.txt` — there is no package manager config:
- `imgui` — static lib built from `third_party/imgui` + the **GLFW** and
  **OpenGL3** backends. Links `glfw3` + `OpenGL::GL`.
- `implot` — static lib, links `imgui` publicly.
- `PocketFFT` — header-only `INTERFACE` library wrapping
  `third_party/pocketfft` (include `<pocketfft_hdronly.h>`).

The main executable has `WIN32_EXECUTABLE` set, so on Windows it runs as a GUI
app with **no console window** — send diagnostic output elsewhere or drop into
a test target.

## Operational notes

- `utils/font_shrink/` holds a small Python tool (`fonttools`) for subsetting
  fonts used by ImGui. It is **not** tracked by git and is not part of the
  build — auxiliary only.
- `build/` and `.cache/` are gitignored; rebuild from scratch if anything in
  `third_party/` changes.
