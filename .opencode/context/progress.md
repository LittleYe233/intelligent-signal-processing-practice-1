# Progress

> ⚠️ **Mistake Distillation**: You must distill the mistakes written here and never make them again in the future.

## Project

**ISPPracticeOne** — C++20 signal frequency estimation simulation framework (MSYS2 UCRT64).
Authoritative plan: `.opencode/context/development_solution.md` (15 sections, milestones M1–M6).

## Current Status

### Commits (local, NOT pushed)

| Hash | Message |
|---|---|
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

### What's Implemented (code on disk, ALL COMPLETE except MUSIC/ESPRIT algorithm)

**Core layer**:
- `src/core/rng.cpp` — ✅ four distribution samplers (normal/uniform/laplace/impulse)
- `src/core/fft.cpp` — ✅ `computeDft` (PocketFFT r2c, normalized ×2/N) + `findPeaksFromDft`
- `include/ispp/core/types.h` — `RealArray`, `ComplexArray`, `FrequencyPeak`, `EstimationResult`, `AMP_UNKNOWN`
- `include/ispp/core/parameters.h` — `SignalSpec`, `WindowKind`, `NoiseSpec`, `NoiseInfo`, `InterferenceSpec`, `EnvSpec`
- `include/ispp/estimator/estimator.h` — `EstimationContext` struct + `IEstimator` interface

**Signal/Window layer**:
- `src/signal/signal_generator.cpp` — ✅ sine + interference + noise (SNR-scaled, 4 distributions)
- `src/window/window.cpp` — ✅ Rectangular/Hamming/Hann/Blackman (user implemented)

**Estimator layer**:
- `src/estimator/fft_peak.cpp` — ✅ PocketFFT + threshold peak search (user implemented)
- `src/estimator/fft_interpolate.cpp` — ✅ Quinn init + binary search refinement (user implemented)
- `src/estimator/music.cpp` — ⏳ skeleton stub (user must implement with Eigen)
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
1. **`src/estimator/music.cpp`** — implement MUSIC algorithm (needs Eigen SVD)
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

### Naming Conventions (enforced by `.clang-tidy`)
- `CamelCase`: classes, structs, enums, members (`Peaks`, `Config`, `Worker`) — NO trailing underscore
- `camelBack`: functions (`estimate`, `name`, `addPeaks`)
- `UPPER_CASE`: constants, enum values (`RECTANGULAR`, `GAUSSIAN`, `AMP_UNKNOWN`)
- `lower_case`: parameters, local variables (`sample_rate`, `max_peak_count`)
- Traditional include guards: `#ifndef ISPP_..._H`

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

**Resolution**: NEVER run clang-format on `CMakeLists.txt` or any non-C++ file.

**Lesson**: Only format `.h`, `.cpp` files. CMake, Markdown, and other formats must be hand-edited.

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

## Build & Validation Commands

```powershell
# Configure (canonical, matches VS Code CMake Tools)
cmake -S . -B build -G Ninja -Wno-unused-cli `
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# clang-format (C++ files ONLY — never CMakeLists.txt)
& "C:\msys64\ucrt64\bin\clang-format.exe" -style=file -i <files>

# clang-tidy (needs compile_commands.json from build/)
& "C:\msys64\ucrt64\bin\clang-tidy.exe" "-p=build" "--quiet" <files>
```

**Toolchain**: clang-tidy 22.1.8, GCC from MSYS2 UCRT64 (`C:\msys64\ucrt64\bin`).
**Debug builds**: include `-g` flag (user added to CMakeLists.txt).

## Session Context Files

- `.opencode/context/development_solution.md` — authoritative plan (~870 lines, updated through M5)
