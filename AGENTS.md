# AGENTS.md

Compact orientation for AI agents working in this repo.

## Project

C++20 signal-frequency-estimation simulation app (`ISPPracticeOne`, GUI via
ImGui/ImPlot). `src/main.cpp` bootstraps i18n then calls `UiManager::run()`.
`test/` holds **interactive smoke-tests of third-party libraries**
(ImGui/ImPlot/PocketFFT), not unit tests — treat as exploratory harnesses.
Deep design history (why things are the way they are, ~28 numbered open
questions/decisions) lives in `.opencode/context/development_solution.md`
and `.opencode/context/progress.md` (gitignored, local-only) — read them
before making non-trivial architecture changes; they are the authoritative
record, not this file.

## Toolchain (Windows / MSYS2 only)

Targets **MSYS2 UCRT64**. Requires:
- `gcc`/`g++`, `cmake`, `ninja`, `clang-format`, `clang-tidy` from
  `C:\msys64\ucrt64\bin`
- `glfw3` via `pacman -S mingw-w64-ucrt-x86_64-glfw` (resolved by
  `find_package(glfw3)` — not vendored)
- OpenGL, and `Gettext`/`Intl`/`Iconv` (for i18n; `find_package(...REQUIRED)`)

Fresh checkout needs submodules (`imgui`, `implot`, `pocketfft`, `eigen`):
```sh
git submodule update --init --recursive
```

## Build

Canonical configure/build (matches VS Code CMake Tools setup):
```sh
cmake -S . -B build -G Ninja -Wno-unused-cli \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```
- CMake's own default is `Release`; **always develop in `Debug`** to match
  the working configuration (adds `-g`).
- `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` is required — clangd reads
  `build/compile_commands.json`.
- **Windows exe lock**: if the app (`ISPPracticeOne.exe`) is still running,
  `cmake --build build` compiles fine but fails at the link step with
  `Permission denied`. This is not a code bug — kill the running exe first.
- Changing link options (e.g. static-linking flags) requires a clean
  reconfigure (`Remove-Item -Recurse -Force build`); incremental builds
  don't re-evaluate `add_link_options`.

### Build options (off by default)
- `-DISPP_ENABLE_NATIVE=ON` / `-DISPP_ENABLE_X86_64_V4=ON` /
  `-DISPP_ENABLE_X86_64_V3=ON` — mutually exclusive `-march=` flags, first
  match wins (see `CMakeLists.txt`).
- `-DISPP_WIN32_GUI=ON` sets `WIN32_EXECUTABLE` (no console window). It
  **defaults OFF** during development on purpose — the console must stay
  open to see `std::cout`/gettext diagnostics. Only flip ON for a release
  build, and remember stdout becomes invisible when you do.

## Tests — do not run

`option(BUILD_TESTING OFF)` in the root `CMakeLists.txt` means `test/` is
not even added to the build unless you pass `-DBUILD_TESTING=ON`. **Do not
run `ctest`, do not add unit tests, do not gate work on test results.** The
`test_*.cpp` files are manual exploration of dependencies only — leave them
alone unless explicitly asked.

## Code style (enforced, not advisory)

- **C++20 strict**, `CMAKE_CXX_EXTENSIONS=OFF` — no GNU extensions.
- **Strict warnings**: `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` on
  GCC/Clang. `-Wconversion` rejects mixed signed/unsigned arithmetic
  (`iterator + size_t`, `span[ptrdiff_t]`, etc.) — cast explicitly.
- **Naming** (`.clang-tidy` `readability-identifier-naming`, no exceptions):
  `UPPER_CASE` constants; `CamelCase` classes/enums/**members** (no
  trailing underscore, e.g. `Config`, `Worker`, not `Config_`);
  `camelBack` functions; `lower_case` parameters/locals.
- **clangd is the language server**, not IntelliSense.

### Before any further action after editing code
1. Run **clang-format** on every changed `.h`/`.cpp`/`.tpp` file — but
   **never** on `CMakeLists.txt` or other non-C++ files (clang-format
   corrupts CMake syntax by joining lines).
   - `.tpp` files aren't auto-recognized; format them with
     `clang-format -style=file -assume-filename dummy.h -i file.tpp`
     (use a **space**, not `=`, before `dummy.h` — PowerShell parses
     `-assume-filename=dummy.h` incorrectly).
2. Run **clang-tidy** (needs `build/compile_commands.json`) and resolve
   every diagnostic for the files you touched.

Do not commit, build, or move on until both are clean.

## Architecture

Strict downward dependency layering (upper layers never referenced by
lower ones):
```
UI (src/ui, ImGui/ImPlot panels)
  → Experiment (src/experiment: config, Monte Carlo runner, stats, scan tests)
    → Metrics (src/metrics) / Estimator (src/estimator) / Signal (src/signal) / Window (src/window)
      → Core (include/ispp/core: types, parameters, rng, fft, PeakFinder)
        → PocketFFT | Eigen | ImGui/ImPlot (third_party/)
```
- New estimators implement `IEstimator::estimate(input, EstimationContext)`
  (`include/ispp/estimator/estimator.h`) and return peaks only — timing is
  measured by `ExperimentRunner`, not the estimator.
- New metrics implement `IMetric` (`include/ispp/metrics/metric.h`);
  `isAggregate()`/`finalize()` exist for whole-Monte-Carlo-run metrics
  (see `RelativeEfficiencyMetric`, currently implemented but **not
  registered** — model assumptions don't fit its CRB math yet).
- `PeakFinder<T>` (`include/ispp/core/peak_finder.{h,tpp}`) is the shared
  peak-detection utility (median-filter noise floor → prominence → FWHM).
  It's a template with an all-`static` API; the `.tpp` re-includes its own
  `.h` at the top (standard header-only-template pattern) and is not listed
  in `target_sources` — no CMake wiring needed for new template code here.
  MUSIC/ESPRIT pseudospectra should call `PeakFinder` directly, not
  `findPeaksFromDft` (which assumes linear-magnitude FFT data).
- `src/` mirrors `include/ispp/` 1:1 by subdirectory; new `.cpp` files must
  be added explicitly to `ISPP_SOURCES` in the root `CMakeLists.txt`.

## Dependency wiring

All in `CMakeLists.txt` — no package manager config:
- `imgui` — static lib, `third_party/imgui` + GLFW/OpenGL3 backends.
- `implot` — static lib, links `imgui` publicly.
- `PocketFFT` — header-only `INTERFACE` lib (`<pocketfft_hdronly.h>`).
- `Eigen` — header-only `INTERFACE` lib (used by MUSIC/ESPRIT; SVD /
  `SelfAdjointEigenSolver`). ESPRIT needs `-O3` for reasonable speed.

### Static linking (default on MinGW)
`CMakeLists.txt` adds `-static -static-libgcc -static-libstdc++` whenever
`MINGW` is set (always true on MSYS2 UCRT64), and forces
`CMAKE_FIND_LIBRARY_SUFFIXES ".a"` so the linker prefers `.a` over
`.dll.a`. Result: `build/ISPPracticeOne.exe` statically links GCC runtime,
libstdc++, winpthread, and GLFW; only Windows system DLLs remain dynamic.
Verify with `& 'C:\msys64\ucrt64\bin\ldd.exe' build\ISPPracticeOne.exe` —
only `C:\WINDOWS\system32\*.dll` should appear. Remember: a `STATIC` CMake
library target (e.g. `imgui`) is unrelated to whether the *final exe*
links statically — both settings are needed independently.

## i18n (gettext)

- `include/ispp/i18n.h` defines `_UI(S)`/`_CON(S)` → `dgettext("ui"/"con", S)`
  (two text domains: `ui` for panels, `con` for console). Use domain-scoped
  `dgettext`, not plain `gettext`, at every new call site.
- **Never call `_UI()`/`dgettext()` from a background thread** — it's not
  guaranteed thread-safe. Worker threads (`ExperimentRunner`,
  `ScanTestRunner`) must compare against stable English literals; only the
  UI thread should translate for display.
- `main.cpp` resolves `locales/` relative to the **exe's own directory**
  (`GetModuleFileNameW`), not the current working directory.
- `.po` → `.mo` compilation happens via `cmake/msgfmt.cmake` into
  `${CMAKE_BINARY_DIR}/locales/...`. There is currently no `install()`/copy
  step to place `locales/` next to the exe elsewhere — only running from
  `build/` is verified to find translations.
- Regenerate the `.pot` template with `xgettext` after adding new
  `_UI()`/`_CON()` strings (see `locales/pot/ui.pot` header for the exact
  invocation used previously); update `locales/zh_CN/ui.po` to match.

## Operational notes

- `utils/font_shrink/` (Python/`fonttools`, if present) is an untracked,
  build-independent helper for subsetting ImGui fonts — not part of CMake.
- `build/` and `.cache/` are gitignored; rebuild from scratch if anything
  in `third_party/` changes.
- `.opencode/context/` is local session history (gitignored) — check it
  first when confused about *why* something is implemented a certain way;
  it documents dead ends already tried (e.g. a whole SIGSEGV root-cause
  hunt for the log ring buffer) so you don't repeat them.
