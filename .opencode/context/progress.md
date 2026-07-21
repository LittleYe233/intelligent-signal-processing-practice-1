# Progress

> ⚠️ **Mistake Distillation**: You must distill the mistakes written here and never make them again in the future.

## Project

**ISPPracticeOne** — C++20 signal frequency estimation simulation framework (MSYS2 UCRT64).
Authoritative plan: `.opencode/context/development_solution.md` (15 sections, milestones M1–M6, doc v1.1).

## Current Status

### Commits (local, NOT pushed)

| Hash | Message |
|---|---|
| `05d18bb` | build(cmake): refine build options |
| `388e990` | ♻️ refactor(core): extract peak-finding into reusable PeakFinder utility |
| `4d7fa6a` | fix(fft_interpolate): fix formula error |
| `c8afafb` | fix(fft_interpolate): fix vector index selection error |
| `90d3faa` | chore(context): rename development_plan to development_solution |
| `bf920b2` | ✨ feat(ui): implement complete M5 UI layer with panels and exception handling |
| `0b3184c` | ✨ feat(metrics): implement three evaluation metrics |
| `7cf531b` | ✨ feat(window): implement four standard window functions |
| `f146d7a` | ✨ feat(signal): implement signal generation pipeline |
| `1cf2961` | ✨ feat(core): implement RNG distribution samplers |
| `581cffd` | ✨ feat(estimator): implement FFT interpolation estimator |
| `634d9ad` | 🏗️ refactor: consolidate estimator signal context into EstimationContext |
| `41d07d0` | 🏗️ refactor: extract core/fft utilities and add WindowKind to estimator interface |
| `6c36be8` | ✨ feat(estimator): add M2 FFT estimators with tightened estimate() interface |
| `7875d71` | ✨ feat(framework): add M1 skeleton with complete MonteCarlo runner |
| `e2a8ab9` | chore(clang-tidy): refine .clang-tidy |
| `1293107` | build(deps): add library Eigen |
| `be7fb28` | chore(agent): add AGENTS.md and development_solution.md |

All commits are local on `main`. **Not pushed** — user explicitly said "never git-push".

### Milestone Progress

| # | Milestone | Status |
|---|---|---|
| M1 | Core types + Signal/Window skeleton + MonteCarlo complete | ✅ Done (`7875d71`) |
| M2 | FFT estimators + core/fft + rng + signal + window + metrics | ✅ Done (multiple commits) |
| M3 | MUSIC/ESPRIT skeletons | ✅ Skeletons created (`bf920b2`) — algorithm logic is user responsibility |
| M4 | (merged into M2) Metrics + Statistics | ✅ Done (`0b3184c`) |
| M5 | UI complete + main.cpp | ✅ Done (`bf920b2`) |
| M6 | End-to-end smoke test | ⏳ Pending (user runs the app) |

**Refactor work outside milestones**:
- PeakFinder utility extracted from `findPeaksFromDft` (`388e990`). Doc tracked as v1.1 in `development_solution.md` (§5.5, OQ-8/9/10) but **not** as a formal milestone per user instruction.

### Pending (uncommitted, this session)

- **`CMakeLists.txt`** — added "Static Linking" section (`if(MINGW) add_link_options(-static -static-libgcc -static-libstdc++) endif()`). Forces linker to prefer `.a` over `.dll.a` for GCC runtime, stdlib, winpthread, and GLFW. Resulting `build/ISPPracticeOne.exe` is self-contained, runs on any Windows 10+ machine without MSYS2. **User verified via `ldd`** — no MSYS2 DLLs in dependency list. Suggested commit message: `build(cmake): static-link runtime + GLFW for redistributable exe`.
- **`AGENTS.md`** — added "Static linking (default on MinGW)" subsection under "Dependency wiring".

### What's Implemented (code on disk, ALL COMPLETE except MUSIC/ESPRIT algorithm)

**Core layer**:
- `src/core/rng.cpp` — ✅ four distribution samplers (normal/uniform/laplace/impulse)
- `src/core/fft.cpp` — ✅ `computeDft` (PocketFFT r2c, normalized ×2/N) + `findPeaksFromDft` (delegates to `PeakFinder<double>`)
- `include/ispp/core/peak_finder.h` — ✅ `PeakFinder<T>` class template (static-only API, nested `Peak` struct) — NEW in `388e990`
- `include/ispp/core/peak_finder.tpp` — ✅ template impl: median filter + prominence + FWHM + `findPeaks` pipeline (user implemented) — NEW in `388e990`
- `include/ispp/core/types.h` — `RealArray`, `ComplexArray`, `FrequencyPeak`, `EstimationResult`, `AMP_UNKNOWN`
- `include/ispp/core/parameters.h` — `SignalSpec`, `WindowKind`, `NoiseSpec`, `NoiseInfo`, `InterferenceSpec`, `EnvSpec`
- `include/ispp/estimator/estimator.h` — `EstimationContext` struct + `IEstimator` interface

**Signal/Window layer**:
- `src/signal/signal_generator.cpp` — ✅ sine + interference + noise (SNR-scaled, 4 distributions)
- `src/window/window.cpp` — ✅ Rectangular/Hamming/Hann/Blackman (user implemented)

**Estimator layer**:
- `src/estimator/fft_peak.cpp` — ✅ PocketFFT + threshold peak search (user implemented)
- `src/estimator/fft_interpolate.cpp` — ✅ Quinn init + binary search refinement (user implemented; two formula/index bugfixes in `c8afafb` + `4d7fa6a`)
- `src/estimator/music.cpp` — ⏳ skeleton stub (user must implement with Eigen; can use `PeakFinder` on pseudospectrum directly per §6.3)
- `src/estimator/esprit.cpp` — ⏳ skeleton stub (user must implement with Eigen)

**Metrics layer**:
- `src/metrics/percentage_error.cpp` — ✅ `|Δf|/f_true × 100%` via min-error peak (OQ-6)
- `src/metrics/rmse.cpp` — ✅ returns `(Δf)²` (MC mean = MSE)
- `src/metrics/compute_time.cpp` — ✅ returns `result.ComputeTimeSec`

**Experiment layer**:
- `src/experiment/statistics.cpp` — ✅ mean/std/min/max
- `src/experiment/experiment_runner.cpp` — ✅ full MonteCarlo loop, builds `EstimationContext`, assembles `EstimationResult`

**UI layer (M5)**:
- `src/ui/ui_manager.cpp` — ✅ GLFW/ImGui/ImPlot init, DPI-normalized sizing, main loop, background thread, try-catch
- `src/ui/panels/config_panel.cpp` — ✅ all config controls with PushID/PopID
- `src/ui/panels/spectrum_panel.cpp` — ✅ time domain + frequency domain dB + peaks + true freq line
- `src/ui/panels/results_panel.cpp` — ✅ metrics table + peak info
- `src/ui/panels/log_panel.cpp` — ✅ thread-safe ring buffer (200 messages)
- `src/ui/widgets/enum_combo.h` — ✅ template enum ↔ ImGui::Combo bridge
- `src/main.cpp` — ✅ calls `UiManager::run()`

### User Responsibilities (REMAINING)

Only two items left:
1. **`src/estimator/music.cpp`** — implement MUSIC algorithm (needs Eigen SVD). Use `PeakFinder<double>::findPeaks` directly on pseudospectrum (NOT `findPeaksFromDft`).
2. **`src/estimator/esprit.cpp`** — implement ESPRIT algorithm (needs Eigen SVD + subspace rotation)

Everything else is complete and operational.

## Key Architecture Decisions

### Estimator Interface (Option B + EstimationContext)

```cpp
struct EstimationContext {
    double SampleRateHz;
    WindowKind WindowKind;
    std::size_t FrequencyCount;  // = MaxFreqCount + (interference ? 1 : 0)
    NoiseInfo NoiseInfo;
};

class IEstimator {
    virtual std::vector<FrequencyPeak>
    estimate(const RealArray &input, const EstimationContext &context) = 0;
};
```

- Estimators return ONLY peaks (no timing) — Runner assembles `EstimationResult` with measured time
- `input` is **already windowed** — `applyWindow()` mutates in place before `estimate()`
- `FrequencyCount` derived from `MaxFreqCount` + interference presence

### PeakFinder Utility (NEW in `388e990`, doc §5.5, OQ-8/9/10)

```cpp
template <std::floating_point T>
class PeakFinder {
public:
    struct Peak {
        std::size_t Index;
        T Prominence;
    };
    static std::vector<Peak> findPeaks(std::span<const T> data,
                                       std::size_t kernel_size,
                                       T margin, T min_prominence,
                                       T min_width = static_cast<T>(1.0));
private:
    static std::vector<T> calcMedianFilter(...);
    static T calcProminence(...);
    static T calcWidth(...);
};
```

- **Location**: `include/ispp/core/peak_finder.{h,tpp}` — Core layer "工具" slot, FFT-agnostic
- **API**: All methods `static` — pure utility class, no instance state (corrected reference signature's invalid `static ... const`)
- **File layout**: `.tpp` included by `.h` at namespace close; `.tpp` includes `.h` unconditionally at top (include guard prevents actual circular inclusion). `.tpp` is NOT in `target_sources` — header-only template. No CMake changes needed.
- **`findPeaksFromDft` signature unchanged** — public API preserved; delegates internally to `PeakFinder<double>::findPeaks`. Old params mapped:
  - `kernel_size = 31` (fixed)
  - `margin = min_prominence = threshold_factor × max_mag`
  - `min_width = 1.0`
  - `max_peak_count` → post-filter by prominence descending
- **MUSIC/ESPRIT should use `PeakFinder` directly** on pseudospectra, NOT `findPeaksFromDft` (which applies `|dft[i]|` to already-linear data — wrong for pseudospectra)
- **Algorithm**: median-filter noise floor → candidate local maxima → `margin` threshold → prominence ≥ `min_prominence` → FWHM ≥ `min_width`

### Naming Conventions (enforced by `.clang-tidy`)
- `CamelCase`: classes, structs, enums, members (`Peaks`, `Config`, `Worker`, `PeakFinder::Peak::Index`) — NO trailing underscore
- `camelBack`: functions (`estimate`, `name`, `addPeaks`, `findPeaks`, `calcMedianFilter`)
- `UPPER_CASE`: constants, enum values, local `const` variables (`RECTANGULAR`, `GAUSSIAN`, `AMP_UNKNOWN`, `KERNEL_SIZE`)
- `lower_case`: parameters, local variables (`sample_rate`, `max_peak_count`, `peak_idx`)
- Traditional include guards: `#ifndef ISPP_..._H` (and `#ifndef ISPP_..._TPP` for `.tpp` files)

### UI Architecture
- `UiManager` owns GLFW window, ImGui/ImPlot contexts, all panels, and the worker thread
- Worker thread runs `ExperimentRunner::run()` with progress callback; detaches
- Main loop polls for completion via mutex-protected `PendingResult`
- Exception handling wraps both worker thread and main loop (best-effort, logs to LogPanel)

### DPI Normalization (§8.1)
`glfwGetVideoMode` returns physical pixels; must divide by `glfwGetMonitorContentScale` before applying 85%:
```cpp
int win_w = mode->width / monitor_scale_x * 0.85;
```

### Static Linking for Redistribution (NEW this session)

`CMakeLists.txt` gates static linking behind `if(MINGW)`:
```cmake
if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()
```

Effect (verified via `ldd`):
- `libgcc.a` ⟵ instead of `libgcc_s_seh-1.dll`
- `libstdc++.a` ⟵ instead of `libstdc++-6.dll`
- `libwinpthread.a` ⟵ instead of `libwinpthread-1.dll`
- `libglfw3.a` ⟵ instead of `glfw3.dll` (ld prefers `.a` over `.dll.a` under `-static`)

OpenGL (`opengl32.dll`) and Windows system DLLs remain dynamic (they ship with the OS).

**Why not always `-static`?** Potential ABI conflicts when mixing with other DLLs that themselves use libstdc++ — but for a self-contained GUI exe that links nothing else, this is the right tradeoff.

**Caveat**: link-option changes need `Remove-Item -Recurse -Force build`; incremental rebuilds do not re-evaluate them.

## Mistakes & Lessons (NEVER REPEAT)

### 1. `// NOLINTNEXTLINE` placement for multi-line function declarations

**Mistake**: Put `// NOLINTNEXTLINE(...)` before a multi-line function declaration. It only suppressed the first line, not the line where the swappable parameters appeared.

**Resolution**: Place `// NOLINT(check-name)` as a **trailing comment on the SAME line** as the first swappable parameter.

**Lesson**: `NOLINTNEXTLINE` suppresses only the **next single source line**. For multi-line declarations, use inline `// NOLINT(...)` on the exact line of the diagnostic.

### 2. `modernize-use-designated-initializers` for aggregate initialization

**Mistake**: Used positional brace-init: `EstimationResult{std::move(peaks), compute_sec}`.

**Resolution**: clang-tidy 22.x requires designated initializers:
```cpp
EstimationResult{.Peaks = std::move(peaks), .ComputeTimeSec = compute_sec}
```

### 3. clang-format on CMakeLists.txt CORRUPTS the file

**Mistake**: Ran `clang-format -i CMakeLists.txt`. clang-format is for C++ only; running it on CMake syntax joins lines incorrectly ("Expected a newline, got identifier with text 'project'").

**Resolution**: NEVER run `clang-format` on `CMakeLists.txt` or any non-C++ file.

**Lesson**: Only format `.h`, `.cpp`, `.tpp` files. CMake, Markdown, and other formats must be hand-edited.

### 4. ImGui ID conflicts with duplicate labels

**Mistake**: Two `ImGui::InputDouble("Amplitude", ...)` controls in the same window (Signal section + Interference section) caused ID conflicts.

**Resolution**: Wrap each section's controls with `ImGui::PushID("SectionName")` / `ImGui::PopID()`.

**Lesson**: ImGui uses the label string as the control ID. Duplicate labels in the same window stack conflict. Use `PushID`/`PopID` or `##hidden` suffix to disambiguate.

### 5. DPI scaling makes window exceed screen

**Mistake**: `glfwGetVideoMode` returns physical pixels (e.g., 3840×2160). Multiplying by 0.85 and passing to `glfwCreateWindow` (which expects logical/screen coordinates) created a window larger than the visible screen on high-DPI displays.

**Resolution**: Divide by `glfwGetMonitorContentScale` to convert physical→logical pixels before applying 85%.

### 6. `bugprone-easily-swappable-parameters` globally disabled

The user disabled this check globally in `.clang-tidy` (`-bugprone-easily-swappable-parameters`) after dealing with NOLINT comments on multiple constructors. Future code does NOT need NOLINT for this check.

### 7. Member naming: NO trailing underscore

The project convention is plain `CamelCase` for members (`Config`, `Worker`, `Log`), NOT `Config_` or `Worker_`. The trailing underscore style causes `readability-identifier-naming` warnings.

### 8. `static` member functions cannot have cv-qualifiers (NEW in `388e990`)

**Mistake**: User-provided reference signature had `static std::vector<Peak<T>> findPeaks(...) const {}` — the trailing `const` is invalid because static methods have no `this` to be const-qualified.

**Resolution**: Remove `const` from static methods. Make all private helpers `static` too (since class has no instance state).

**Lesson**: When given a "reference signature" by the user, validate it against C++ rules. `static` and cv-qualifiers are mutually exclusive on member functions.

### 9. `.tpp` template implementation files need `#include "parent.h"` (NEW in `388e990`)

**Mistake**: Initial `.tpp` didn't include parent `.h`. Compiling `.cpp` files that included the `.h` worked (because `.h` → `.tpp` chain had full context), but **clangd reported `PeakFinder` template not found** when parsing `.tpp` standalone.

**Resolution**: Add `#include "ispp/core/peak_finder.h"` at top of `.tpp` (inside its own include guard). The `.h`'s include guard prevents actual circular inclusion — when included from `.h`, the `#include "..."h"` is a no-op.

**Lesson**: This is the standard Boost/Eigen pattern for template implementation files. The `.tpp` is never a standalone TU, but editors/IDEs parse it independently — it must be self-contained with respect to declarations it uses.

### 10. `[[maybe_unused]]` placement differs between GCC and clang (NEW in `388e990`)

**Mistake**: Wrote `std::size_t [[maybe_unused]] name` to suppress unused-parameter warnings in stub functions. clangd/clang accepted it; **GCC warned "attribute ignored — an attribute that appertains to a type-specifier is ignored"** AND `-Wunused-parameter` still fired.

**Resolution**: Use `(void)param;` idiom inside the function body. Universally portable across GCC/clang/MSVC.

**Lesson**: For function parameter suppression that needs GCC compatibility, prefer `(void)param;` over `[[maybe_unused]]` on parameter declarations. The attribute placement `Type [[maybe_unused]] name` between type and declarator is interpreted as applying to the type-specifier in GCC, not the parameter.

### 11. Function name mismatch passes clangd but fails GCC (NEW in `388e990`)

**Mistake**: User implementation of `findPeaks` called `calculate_median_filter`, `calculate_prominence`, `calculate_width`, but the header declared `calcMedianFilter`, `calcProminence`, `calcWidth`. **clangd passed without error**; **GCC reported** `error: 'calculate_median_filter' was not declared in this scope; did you mean 'calcMedianFilter'?`.

**Resolution**: Rename call sites to match declared method names exactly.

**Lesson**: clangd is more permissive with unqualified name lookup in templates than GCC. **Always trust the compiler over the language server.** When GCC suggests "did you mean X?", check the call site against the declaration — there's almost always a typo or naming convention mismatch.

### 12. `-Wconversion` on iterator arithmetic and signed/unsigned indexing (NEW in `388e990`)

**Mistake**: User's `.tpp` had three `-Wsign-conversion` violations:
- `data.begin() + end` where `end` is `size_t` (iterator `operator+` wants `difference_type`)
- `data[j]` where `j` is `std::ptrdiff_t` (span `operator[]` wants `size_type`)
- Same pattern in `calcWidth`

**Resolution**:
```cpp
data.begin() + static_cast<std::ptrdiff_t>(end)
const auto UJ = static_cast<std::size_t>(j);
data[UJ]
```

**Lesson**: With `-Wconversion` enabled (project default per `AGENTS.md`), any mixed signed/unsigned arithmetic needs an explicit cast. Common patterns to watch:
- `iterator + size_t` → cast to `std::ptrdiff_t`
- `span[ptrdiff_t]` → cast to `std::size_t`
- `vector[ptrdiff_t]` → cast to `std::size_t`

### 13. `clang-format -assume-filename=value` fails in PowerShell (NEW in `388e990`)

**Mistake**: `clang-format -assume-filename=dummy.h -i file.tpp` returned error `.h: No such file or directory` in PowerShell. The `=` after the flag name was being parsed differently.

**Resolution**: Use space instead of `=`: `clang-format -assume-filename dummy.h -i file.tpp`.

**Lesson**: In MSYS2/PowerShell environments, prefer space-separated flag values over `=`, even for flags documented with `=` syntax.

### 14. `modernize-use-ranges` requires C++20 ranges algorithms (NEW in `388e990`)

**Mistake**: Used `std::sort(v.begin(), v.end(), cmp)` and `std::partial_sort(...)`. clang-tidy flagged `modernize-use-ranges`.

**Resolution**: Use ranges versions:
```cpp
std::ranges::sort(v, {}, proj);              // {} = default comparator, proj = projection
std::ranges::partial_sort(v, middle_iter, cmp);
```

**Lesson**: With C++20 enabled, prefer `std::ranges::*` algorithms. The ranges API supports projections which often eliminate manual comparator lambdas.

### 15. "静态库" (static library) ≠ "静态链接" (static linking) (NEW this session)

**Mistake**: User assumed `add_library(imgui STATIC ...)` made the final exe self-contained (no DLL dependencies). But:
1. imgui's `target_link_libraries(... PUBLIC glfw3 ...)` propagates glfw3 to the final exe, and `find_package(glfw3)` on MSYS2 returns the shared import library (`libglfw3.dll.a`), not the static archive (`libglfw3.a`). Resulting exe depends on `glfw3.dll` at runtime.
2. MinGW defaults to dynamic linking for `libgcc`, `libstdc++`, `libwinpthread` — completely unrelated to whether third-party libs are built as `STATIC`.

Symptom: exe built and ran on dev machine, but on a fresh Windows machine reported missing `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, `glfw3.dll`.

**Resolution**: Add explicit link options in `CMakeLists.txt`:
```cmake
if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()
```
`-static` forces ld to prefer `.a` archives over `.dll.a` import libs globally; `-static-libgcc`/`-static-libstdc++` are belt-and-suspenders for the GCC runtime. Must do a clean reconfigure (`Remove-Item -Recurse -Force build`) — incremental rebuilds do not re-evaluate link options.

**Lesson**: Two orthogonal concepts:
1. **`STATIC` vs `SHARED` library** (CMake target property) — how a *dependency* is built.
2. **`-static` vs default dynamic linking** (linker option) — how the *final executable* binds its dependencies.

A `STATIC` imgui still produces a dynamic exe unless you also pass `-static` at link time. For redistributable Windows exes built with MSYS2 UCRT64, ALWAYS add `-static -static-libgcc -static-libstdc++` (assuming `.a` versions exist — UCRT64 ships them by default; verify with `ls C:\msys64\ucrt64\lib\lib<name>.a`).

**Validation**: DO NOT validate this point.

## Build & Validation Commands

```powershell
# Configure (canonical, matches VS Code CMake Tools)
cmake -S . -B build -G Ninja -Wno-unused-cli `
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# clang-format (C++ files ONLY — never CMakeLists.txt)
& "C:\msys64\ucrt64\bin\clang-format.exe" -style=file -i <files>

# clang-format for .tpp files (use -assume-filename with SPACE, not =)
& "C:\msys64\ucrt64\bin\clang-format.exe" -style=file -assume-filename dummy.h -i <file.tpp>

# clang-tidy (needs compile_commands.json from build/)
& "C:\msys64\ucrt64\bin\clang-tidy.exe" "-p=build" "--quiet" <files>
```

**Toolchain**: clang-tidy 22.1.8, clang-format 22.1.8, GCC from MSYS2 UCRT64 (`C:\msys64\ucrt64\bin`).
**Debug builds**: include `-g` flag (user added to CMakeLists.txt).
**`.tpp` support**: `.clang-tidy` has `HeaderFileExtensions: ['h', 'hh', 'hpp', 'hxx', 'tpp']`; `.clang-format` documents the `-assume-filename` workaround.

## Session Context Files

- `.opencode/context/development_solution.md` — authoritative plan (v1.1, ~1025 lines, updated through PeakFinder refactor §5.5)
- `.opencode/context/progress.md` — this file
- `.tmp/sessions/2026-07-19-m1-core-signal-montecarlo/context.md` — stale session from M1 (safe to delete; M1 is complete)
