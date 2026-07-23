# Progress

> ⚠️ **Mistake Distillation**: You must distill the mistakes written here and never make them again in the future.

## Project

**ISPPracticeOne** — C++20 signal frequency estimation simulation framework (MSYS2 UCRT64).
Authoritative plan: `.opencode/context/development_solution.md` (15 sections, milestones M1–M6, doc v1.5).

## Current Status

### Commits (local, NOT pushed)

| Hash | Message |
|---|---|
| `f9f800d` | ♻️ refactor(signal): remove amplitude parameter, fix to 1.0 |
| `ad94f78` | 🐛 fix(ui): auto-refit spectrum axes on new experiment data |
| `a73421b` | fix(ui): fix double shutdown() error |
| `85e330f` | ✨ feat(metrics): revise metric display with per-metric formatting and correct RMSE |
| `f422992` | ✨ feat(i18n): wrap UI strings in _UI() and add zh_CN translation |
| `de01c29` | feat(i18n): add GNU gettext dep and i18n support |
| `3622839` | build(cmake): link all deps statically |
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
| M3 | MUSIC/ESPRIT | ✅ Done — both implemented by user (beam-space MUSIC + Hankel ESPRIT via Eigen) |
| M4 | (merged into M2) Metrics + Statistics | ✅ Done (`0b3184c`) |
| M5 | UI complete + main.cpp | ✅ Done (`bf920b2`) |
| M6 | End-to-end smoke test | ✅ Done — user verified all 4 algorithms |

**Refactor work outside milestones**:
- PeakFinder utility extracted from `findPeaksFromDft` (`388e990`). Doc tracked as v1.1 in `development_solution.md` (§5.5, OQ-8/9/10) but **not** as a formal milestone per user instruction.
- **i18n layer** (`de01c29` + `f422992`) — GNU gettext integration with two text domains (`ui`/`con`), Win32 locale auto-detection, zh_CN translation. Outside M1–M6 scope; user-initiated enhancement.
- **Metrics revision** (`85e330f`) — removed metric enable/disable checkbox; corrected RMSE (was computing MSE, name was `"MSE"`); added per-metric `format()` + `showDistribution()` to `IMetric`; `RunResult` changed from `unordered_map<string, MetricStats>` to `vector<MetricResult>`. Doc tracked as v1.2 in `development_solution.md` (§6.4, §8.3, §8.5, OQ-11/12/13). Outside M1–M6 scope; user-initiated revision.
- **UI bug fixes** (`a73421b` + `ad94f78`) — fixed double-shutdown segfault on window close (OQ-14); fixed ImPlot axis auto-refit on new experiment data via conditional `ImPlotCond` (OQ-15); added interference frequency reference line (OQ-16); spectrum plots wrapped in resizable `BeginChild` containers. Doc tracked as v1.3 in `development_solution.md` (§7.3, §8.4, §8.7, OQ-14~16).
- **Signal amplitude removal** (`f9f800d`) — removed `SignalSpec::Amplitude`; fixed to 1.0; interference amplitude now implicitly relative. Doc tracked as v1.4 (§5.2, §6.2, §8.3, OQ-17).
- **Metrics architecture redesign** (uncommitted, this session, doc v1.5 OQ-18~21) — three changes fully implemented:
  1. `FrequencyPeak` gains `Prominence` field + `PROMINENCE_UNKNOWN` sentinel (OQ-18) — `types.h`, `fft.cpp` (`.Prominence = p.Prominence`), `fft_interpolate.cpp` (`.Prominence = PROMINENCE_UNKNOWN`)
  2. `RmseMetric` → `MseMetric`: files renamed `rmse.{h,cpp}` → `mse.{h,cpp}`; `format()` no sqrt; `name()` → `"MSE"`; peak selection changed to max-Prominence (OQ-19 + OQ-21)
  3. New `RelativeEfficiencyMetric` (aggregate): `η = CRB/SampleVariance`; `IMetric` extended with `isAggregate()` / `finalize()`. **⚠️ Currently DISABLED** — model assumptions don't match CRB conditions; code retained but not wired into Runner / UI

### Working tree

**Pending (uncommitted)** — RelativeEfficiency metric disabled + doc sync:

**Modified files (4)**:
- `src/experiment/experiment_runner.cpp` — removed aggregate metric wiring (`has_aggregate` / `freq_estimates` / `finalize()` call); restored simple evaluate+computeStats loop
- `src/ui/panels/config_panel.cpp` — removed `RelativeEfficiencyMetric` include + registration (back to 3 metrics)
- `include/ispp/metrics/relative_efficiency.h` — added disabled `@note` + re-enablement instructions
- `src/metrics/relative_efficiency.cpp` — added disabled comment header

**Doc/context (2)**:
- `.opencode/context/development_solution.md` — §6.4 table + §8.3 + OQ-20 updated to reflect disabled status
- `.opencode/context/progress.md` — this file

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
- `src/estimator/music.cpp` — ✅ **Implemented** (user). Beam-space MUSIC via Eigen SVD: snapshot matrix → DFT beam-forming → covariance → eigendecomposition → noise subspace projection → pseudospectrum peak search. Uses `findPeaksFromDft` for initial frequency range, then refines. Returns `AMP_UNKNOWN` + `PROMINENCE_UNKNOWN`.
- `src/estimator/esprit.cpp` — ✅ **Implemented** (user). ESPRIT via Hankel data matrix + sliding window + Hermitian eigensolver + subspace rotation. Uses `SelfAdjointEigenSolver` for covariance decomposition (faster than full SVD). References paper 10.1109/FOCS61266.2024.00137. Returns `AMP_UNKNOWN` + `PROMINENCE_UNKNOWN`. Needs `-O3` for reasonable speed.

**Metrics layer** (revised in `85e330f` + v1.5 architecture implemented this session):
- `src/metrics/percentage_error.cpp` — ✅ `|Δf|/f_true × 100%` via min-error peak (OQ-6); `format()` = 4 decimal places + `%` suffix
- `src/metrics/mse.cpp` — ✅ (was `rmse.cpp`) returns `(Δf)²` per iteration; MC mean `= MSE = 1/M·Σ(Δf)²`; `format()` = `{:.6e}` (**no sqrt**); `showDistribution() = false`; peak selection = max-Prominence (OQ-21)
- `src/metrics/compute_time.cpp` — ✅ returns `result.ComputeTimeSec`; `format()` = SI units (`ns`/`us`/`ms`/`s`) + 3 significant digits
- `src/metrics/relative_efficiency.cpp` — ⏳ **DISABLED** (OQ-20). Aggregate metric: `η = CRB/SampleVariance`; `isAggregate() = true`. Code retained and compiled but **not registered** in Runner or config panel. Current model assumptions don't match CRB regularity conditions. To re-enable: restore aggregate handling in `experiment_runner.cpp` + register in `config_panel.cpp`

**Experiment layer**:
- `src/experiment/statistics.cpp` — ✅ mean/std/min/max
- `src/experiment/experiment_runner.cpp` — ✅ full MonteCarlo loop, builds `EstimationContext`, assembles `EstimationResult`

**UI layer (M5)**:
- `src/ui/ui_manager.cpp` — ✅ GLFW/ImGui/ImPlot init, DPI-normalized sizing, main loop, background thread, try-catch
- `src/ui/panels/config_panel.cpp` — ✅ all config controls with PushID/PopID
- `src/ui/panels/spectrum_panel.cpp` — ✅ time domain + frequency domain dB + peaks + true freq line + interference freq line (OQ-16); resizable `BeginChild` layout; conditional axis auto-refit via `ImPlotCond` pointer-change detection (OQ-15); X-axis tight + Y-axis 8% padding
- `src/ui/panels/results_panel.cpp` — ✅ metrics table + peak info
- `src/ui/panels/log_panel.cpp` — ✅ thread-safe ring buffer (200 messages)
- `src/ui/widgets/enum_combo.h` — ✅ template enum ↔ ImGui::Combo bridge
- `src/main.cpp` — ✅ i18n bootstrap (Win32 locale detection, `setlocale`, `bindtextdomain` for `ui`/`con`) THEN `UiManager::run()`

**i18n layer (NEW, `de01c29` + `f422992`)**:
- `include/ispp/i18n.h` — ✅ `_UI(S)` / `_CON(S)` macros → `dgettext("ui", S)` / `dgettext("con", S)` (dual text-domain design, not the default-domain `gettext`)
- `cmake/msgfmt.cmake` — ✅ `find_package(Gettext)`; compiles each `locales/<lang>/<dom>.po` → `${BUILD_DIR}/locales/<lang>/LC_MESSAGES/<dom>.mo` via `${GETTEXT_MSGFMT_EXECUTABLE}`; declares `translations` custom target and wires it as a dependency of `${PROJECT_NAME}`
- `locales/pot/ui.pot` — ✅ xgettext-extracted POT template covering every UI msgid (auto-generated, regenerates via `xgettext -o locales/pot/ui.pot ...`)
- `locales/zh_CN/ui.po` — ✅ Simplified Chinese translations, full coverage of all `ui`-domain msgids
- `CMakeLists.txt` — ✅ `find_package(Intl REQUIRED)` + `find_package(Iconv REQUIRED)`; links `Intl::Intl` + `Iconv::Iconv` to main exe; `include(cmake/msgfmt.cmake)`; flipped `ISPP_WIN32_GUI` default **OFF** (was ON); added `set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")` + `BUILD_SHARED_LIBS OFF` in MINGW block (belt-and-suspenders for static linking — extends Lesson 15)
- All UI panel `.cpp` files — ✅ user-facing literals wrapped with `_UI()` (config / log / results / spectrum panels)
- `src/ui/ui_manager.cpp` — ✅ font bumped `14.0f * xscale` → fixed `16.0f`; added `ImGui::GetStyle().FontScaleMain = 1.5`; added window/scale debug `std::cout << std::format(...)` logging

### User Responsibilities

All complete:

1. **`src/estimator/music.cpp`** — ✅ DONE (beam-space MUSIC via Eigen SVD).
2. **`src/estimator/esprit.cpp`** — ✅ DONE (Hankel matrix + subspace rotation via Eigen).

**All milestones M1–M6 are complete.** The framework is fully operational with all 4 frequency estimation algorithms implemented.

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

### i18n Architecture (NEW, `de01c29` + `f422992`)

**Two text domains** — `ui` for ImGui panels, `con` for console output. Macros use `dgettext("ui"/"con", S)` (domain-specific), **not** `gettext(S)` (default domain), so each call site picks the correct catalog.

**Bootstrap** (`src/main.cpp`, Win32-specific paths gated by `#ifdef _WIN32`):
1. `initWindowsLocale()` — `SetConsoleCP/SetConsoleOutputCP(CP_UTF8)`; if neither `LANG` nor `LC_ALL` is set in env (pwsh often leaves them unset), read `GetUserDefaultLocaleName` (returns e.g. `zh-CN`), convert dash → underscore, append `.UTF-8` → `zh_CN.UTF-8`, set via `_putenv_s("LANG", ...)`.
2. `getExecutableDir()` via `GetModuleFileNameW(nullptr, ...)` — resolves `locales/` relative to the **exe location**, not the CWD. Critical when the app is launched from a different working directory.
3. `setlocale(LC_ALL, "")` → `bindtextdomain("ui", locales_dir)` + `bind_textdomain_codeset("ui", "UTF-8")` → same for `"con"` → `UiManager::run()`.

**Build wiring** (`cmake/msgfmt.cmake`): `find_package(Gettext REQUIRED)`; for each `LANG ∈ {zh_CN}` × `DOM ∈ {ui}`, compile `locales/<lang>/<dom>.po` → `${CMAKE_BINARY_DIR}/locales/<lang>/LC_MESSAGES/<dom>.mo` via `${GETTEXT_MSGFMT_EXECUTABLE}`; `add_custom_target(translations ALL DEPENDS ${MO_FILES})`; `add_dependencies(${PROJECT_NAME} translations)` ensures `.mo` files are built before the exe.

**`locales_dir` at runtime**: `${CMAKE_BINARY_DIR}/locales` is where the `.mo` files land, but `main.cpp` looks up `<exe_dir>/locales`. The build must copy/install the `locales/` tree next to the exe for translations to actually load. (Currently no `install()` rule wires this — user runs from `build/` so the mismatch may be latent. Worth verifying in M6 smoke test.)

**Build option flip**: `ISPP_WIN32_GUI` default is now **OFF** (was ON). The console window stays open so gettext warnings and the `std::cout << std::format(...)` debug output in `ui_manager.cpp` are actually visible. ⚠️ This contradicts the AGENTS.md note about "no console window"; **AGENTS.md has NOT been updated for this flip** — flag for correction.

**Label renames bundled with i18n wiring** (in `f422992`):
- Outer `"Spectrum"` window → `"Single Simulation"`
- `"Time Domain"` plot → `"Waveform"`
- `"Spectrum (dB)"` plot → `"Spectrum"`
- `"Results"` window → `"Results & Metrics"`
- `"No results — run an experiment first."` → `"No data — run an experiment first."`

### Metrics Formatting & Distribution (`85e330f`, doc v1.2 OQ-11/12/13)

**Three changes to the metrics subsystem**:

1. **Always-on metrics** (OQ-11): Removed the `MetricsMask` checkbox section from `ConfigPanel`. All three metrics (PercentageError, RMSE, ComputeTime) are always registered. The `metric_names` local array and the `ImGui::SeparatorText("Metrics")` + checkbox loop were deleted.

2. **RMSE correctness** (OQ-12): `RmseMetric::evaluate()` still returns `(Δf)²` per iteration (mathematically correct — RMSE requires squared errors as input). The Runner computes `mean = MSE` over MC iterations. `RmseMetric::format(value)` applies `std::sqrt(value)` → displays the actual RMSE. `name()` changed from `"MSE"` to `"RMSE"`. `showDistribution() = false` — the results panel renders RMSE as a single-line value (`"RMSE: 1.234e-06"`), NOT a table row, because showing mean/std/min/max of squared errors is mathematically meaningless.

3. **Per-metric formatting** (OQ-13): `IMetric` gained `virtual std::string format(double value) const = 0`. Each metric formats its own values:
   - **PercentageError**: `std::format("{:.3f}%", value)` — three decimal places + `%` suffix (e.g., `0.150%`, `12.345%`)
   - **RMSE**: `std::format("{:.6e}", std::sqrt(value))` — sqrt of MSE, then scientific notation
   - **ComputeTime**: SI unit selection by magnitude (`ns`/`us`/`ms`/`s`) + 3 significant digits (e.g., `375ns`, `5.52us`, `10.5ms`, `1.23s`). Decimal places depend on scaled magnitude: `≥100` → 0 decimals, `≥10` → 1 decimal, `<10` → 2 decimals.

**RunResult data structure change**: `PerMetricStats` (`unordered_map<string, MetricStats>`) was replaced by `Metrics` (`vector<MetricResult>`), where `MetricResult = { shared_ptr<IMetric> MetricObj, MetricStats Stats }`. This threads the metric objects through to the results panel so it can call `format()` and `showDistribution()`. Side benefit: insertion order is now preserved (unordered_map had undefined iteration order).

**ResultsPanel rendering**: two-pass — first renders a `BeginTable` for all `showDistribution() == true` metrics (table row per metric, each cell via `metric->format()`), then renders single-line values for `showDistribution() == false` metrics (currently just RMSE).

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

### 16. `progress.md` "Pending" section must be cleared when its items are committed (NEW this session)

**Mistake**: The previous session listed the CMake static-linking work and the AGENTS.md subsection under "Pending (uncommitted, this session)". That work was then committed in `3622839`, which updated `progress.md` (+50 lines, adding the Static Linking architecture note + Lesson 15) — but the commit **did not remove or mark the "Pending" section as resolved**. The section stayed stale for ~1 day, causing `/resume-progress` to initially report pending work that was already in HEAD.

**Resolution**: When committing an item that `progress.md` lists as pending, either (a) update `progress.md` in the same commit to clear/move the item, or (b) explicitly note in the section that the items are now committed (with the commit hash). Do not leave the section describing a state that no longer matches the working tree.

**Lesson**: `progress.md` is the source of truth that future sessions read first. Any section that contradicts `git status` / `git log` is worse than no section at all. Treat "Pending (uncommitted)" as a queue: items come out when they land in a commit. Re-verify the section against `git log --oneline` before saving.

### 17. Windows exe-lock blocks the linker (NEW this session)

**Mistake**: Ran `cmake --build build` while `ISPPracticeOne.exe` was still running (PID from a previous manual launch). All 10 object files compiled successfully, but the final link step failed with `ld.exe: cannot open output file ISPPracticeOne.exe: Permission denied`. The build reported `FAILED: [code=1]` even though no code was wrong.

**Resolution**: Kill the running process first (`Stop-Process -Name ISPPracticeOne -Force`), then re-run `cmake --build build`. Only the link step re-executes (objects are cached by Ninja).

**Lesson**: On Windows, the linker cannot overwrite a `.exe` that is currently executing. Before invoking a build, check `Get-Process -Name ISPPracticeOne -ErrorAction SilentlyContinue`. If a stale `FAILED: Permission denied` appears at the link step with all objects compiled clean, the code is fine — just kill the process and rebuild. Do NOT waste time hunting for code bugs when the error message specifically says `Permission denied` on the output file.

### 18. ImPlot `SetupAxisLimits` defaults to `ImPlotCond_Once` — silently ignores subsequent calls (NEW this session)

**Mistake**: Called `ImPlot::SetupAxisLimits(ImAxis_Y1, newMin, newMax)` every frame with freshly computed min/max from the latest experiment data. Expected the axis to refit on each new experiment. In reality, only the **first frame** applied the limits; all subsequent calls were silently ignored because the default condition is `ImPlotCond_Once`.

**Root cause**: ImPlot's `ApplyNextPlotData` (line 2162) checks `if (!plot.Initialized || npd_rngc == ImPlotCond_Always)`. After the first frame, `Initialized` is true and the condition is `Once`, so `axis.SetRange()` is never called again.

**Resolution**: Track the input signal's heap pointer (`LastInputSignal.data()`) to detect when `UiManager` move-assigns a fresh `RunResult`. On the data-change frame, pass `ImPlotCond_Always`; on all other frames, pass `ImPlotCond_Once` (no-op, preserves user zoom/pan).

**Lesson**: `ImPlotCond_Once` is not "apply once per call" — it is "apply once per plot ID lifetime". Any `SetupAxisLimits` call after the first frame is dead code unless you use `ImPlotCond_Always`. But `Always` locks the axis every frame (breaks manual zoom). The correct pattern for "refit on data change, preserve user zoom otherwise" is a per-frame condition switch based on data-change detection.

### 19. RAII destructor + explicit `shutdown()` in `run()` = double-shutdown crash (NEW this session)

**Mistake**: `UiManager::run()` called `shutdown()` at the end of the main loop, then `~UiManager()` called `shutdown()` again. The second call hit `ImGui_ImplOpenGL3_Shutdown()` whose backend data (`bd`) was already freed by `IM_DELETE(bd)` in the first call → assertion failure: `bd != nullptr && "No renderer backend to shutdown, or already shutdown?"`.

**Resolution**: Remove the `shutdown()` call from `run()`. The destructor is the sole cleanup point (RAII). Since `app` is a stack variable in `main()`, the destructor runs immediately after `run()` returns — no resource leak window.

**Lesson**: When a class has both an explicit lifecycle method (`run()`) and a destructor that calls the same cleanup (`shutdown()`), calling cleanup from both paths causes double-free. Pick one cleanup invocation point — preferably the destructor (RAII) — and make the lifecycle method NOT call cleanup. If idempotent shutdown is needed (e.g., `run()` might be called multiple times), add a guard flag; but the simplest fix is to trust RAII.

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

- `.opencode/context/development_solution.md` — authoritative plan (**v1.5**, ~1136 lines). Updated through: PeakFinder §5.5 (v1.1); metrics revision §6.4/§8.3/§8.5/OQ-11~13 (v1.2); RunResult struct sync §7.3, spectrum panel §8.4, RAII shutdown §8.7, OQ-14~16 (v1.3); signal amplitude removal §5.2/§6.2/§8.3/OQ-17 (v1.4); FrequencyPeak.Prominence §5.1/§5.4/§6.3, MSE rename §6.4/§8.3/§8.5/OQ-19, RelativeEfficiency + IMetric aggregate extension §6.4/§6.4.1/§7.4/OQ-20 (v1.5). **i18n layer NOT documented here** — it is user-initiated scope outside M1–M6.
- `.opencode/context/progress.md` — this file
- `.tmp/sessions/2026-07-19-m1-core-signal-montecarlo/context.md` — stale session from M1 (safe to delete; M1 is complete)
