# Progress

> ⚠️ **Mistake Distillation**: You must distill the mistakes written here and never make them again in the future.

## Project

**ISPPracticeOne** — C++20 signal frequency estimation simulation framework (MSYS2 UCRT64).
Authoritative plan: `.opencode/context/development_plan.md` (15 sections, milestones M1–M6).

## Current Status

### Commits (local, NOT pushed)

| Hash | Message |
|---|---|
| `6c36be8` | ✨ feat(estimator): add M2 FFT estimators with tightened estimate() interface |
| `7875d71` | ✨ feat(framework): add M1 skeleton with complete MonteCarlo runner |
| `e2a8ab9` | chore(clang-tidy): refine .clang-tidy |
| `1293107` | build(deps): add library Eigen |
| `be7fb28` | chore(agent): add AGENTS.md and development_plan.md |

All commits are local on `main`. **Not pushed** — user explicitly said "never git-push".

### Milestone Progress

| # | Milestone | Status |
|---|---|---|
| M1 | Core types + Signal/Window skeleton + MonteCarlo complete | ✅ Done (committed `7875d71`) |
| M2 | FFT estimators + interface refactor | ✅ Done (committed `6c36be8`) |
| M3 | MUSIC/ESPRIT skeletons | ⏳ Not started |
| M4 | Metrics skeletons + Statistics complete | ⏳ Not started |
| M5 | UI complete + main.cpp | ⏳ Not started |
| M6 | End-to-end smoke test | ⏳ Not started |

### What's Implemented (code on disk)

**Complete implementations** (ready to use):
- `src/experiment/statistics.cpp` — mean/std/min/max via `std::ranges::minmax_element`
- `src/experiment/experiment_runner.cpp` — full MonteCarlo loop with cancellation, progress callback, PocketFFT spectrum caching, **assembles EstimationResult from peaks + timing**
- `src/estimator/fft_peak.cpp` — **complete** PocketFFT r2c + threshold-based local maxima detection (user wrote algorithm logic)

**Skeletons with `/// @todo`** (user must implement):
- `src/core/rng.cpp` — distribution samplers (normal/uniform/laplace/impulse)
- `src/window/window.cpp` — window coefficients (Rectangular/Hamming/Hann/Blackman)
- `src/signal/signal_generator.cpp` — signal synthesis pipeline (sine + interference + noise)
- `src/estimator/fft_interpolate.cpp` — parabolic/Quinn interpolation (**skeleton only, NOT implemented**)
- `src/metrics/*.cpp` — not yet created (M4)

**User responsibilities still pending**:
- Implement all skeletons above
- Add Eigen submodule + CMake integration (MUSIC/ESPRIT need it)
- Implement MUSIC/ESPRIT estimators (M3)

## Key Architecture Decisions

### Estimator Interface (Option B — refactored in M2)

**Original**: `IEstimator::estimate()` returned `EstimationResult` (bundling peaks + compute time).
**Problem**: Estimators cannot meaningfully fill `ComputeTimeSec` — the Runner owns the timing boundary which includes windowing preprocessing (per OQ-5 decision: "算法时间测量边界 = 计入窗函数施加时间").

**Refactored contract**:
```cpp
// Estimator returns ONLY peaks — no timing
virtual std::vector<FrequencyPeak> estimate(const RealArray& input, double sample_rate) = 0;

// Runner assembles the full result with externally measured timing:
auto peaks = Estimator->estimate(input, SAMPLE_RATE);
auto compute_end = std::chrono::steady_clock::now();
EstimationResult est_result{.Peaks = std::move(peaks), .ComputeTimeSec = compute_sec};
```

**`input` parameter semantics**: The `input` passed to `estimate()` is **already windowed** — `applyWindow(input, kind)` mutates in place before `estimate()` is called. See `experiment_runner.cpp` lines 57–62.

### Naming Conventions (enforced by `.clang-tidy`)
- `CamelCase`: classes, structs, enums, members (`Peaks`, `ComputeTimeSec`, `MaxPeakCount`)
- `camelBack`: functions (`estimate`, `name`, `addPeaks`)
- `UPPER_CASE`: constants, enum values
- `lower_case`: parameters, local variables (`sample_rate`, `max_peak_count`)
- Traditional include guards: `#ifndef ISPP_..._H`

## Mistakes & Lessons (NEVER REPEAT)

### 1. `// NOLINTNEXTLINE` placement for multi-line function declarations

**Mistake**: Put `// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)` before a multi-line function declaration. It only suppressed the first line, not the line where the swappable parameters actually appeared.

**Resolution**: Place `// NOLINT(check-name)` as a **trailing comment on the SAME line** as the first swappable parameter:
```cpp
void addPeaks(const std::vector<std::complex<double>> &dft,
              std::vector<ispp::FrequencyPeak> &peaks,
              double threshold_factor, // NOLINT(bugprone-easily-swappable-parameters)
              double fft_rez, std::size_t max_peak_count) {
```

**Lesson**: `NOLINTNEXTLINE` suppresses only the **next single source line**. For multi-line declarations where the diagnostic fires on a later line, use inline `// NOLINT(...)` on the exact line of the diagnostic.

### 2. `modernize-use-designated-initializers` for aggregate initialization

**Mistake**: Used positional brace-init for an aggregate: `EstimationResult{std::move(peaks), compute_sec}`.

**Resolution**: clang-tidy 22.x requires designated initializers for aggregates:
```cpp
EstimationResult est_result{.Peaks = std::move(peaks), .ComputeTimeSec = compute_sec};
```

**Lesson**: Under strict `modernize-*` checks, always use designated initializers `.Field = value` for aggregate struct initialization.

### 3. Committing with `utils/` directory

**Near-miss (caught)**: `utils/font_shrink/` is untracked and must NEVER be committed — it's auxiliary per AGENTS.md ("not tracked by git and is not part of the build").

**Lesson**: Before `git add .`, verify `utils/` is excluded. Prefer explicit staging or rely on the user's pre-staged files.

## Build & Validation Commands

```powershell
# Configure (canonical, matches VS Code CMake Tools)
cmake -S . -B build -G Ninja -Wno-unused-ci `
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# clang-format
& "C:\msys64\ucrt64\bin\clang-format.exe" -style=file -i <files>

# clang-tidy (needs compile_commands.json from build/)
& "C:\msys64\ucrt64\bin\clang-tidy.exe" "-p=build" "--quiet" <files>
```

**Toolchain**: clang-tidy 22.1.8, GCC from MSYS2 UCRT64 (`C:\msys64\ucrt64\bin`).

## Session Context Files

- `.opencode/context/development_plan.md` — authoritative plan, 768 lines
- `.tmp/sessions/2026-07-19-m1-core-signal-montecarlo/context.md` — M1 session context (may be stale)
