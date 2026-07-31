# Progress

> ⚠️ **Mistake Distillation**: You must distill the mistakes written here and never make them again in the future.

## Project

**ISPPracticeOne** — C++20 signal frequency estimation simulation framework (MSYS2 UCRT64).
Authoritative plan: `.opencode/context/development_solution.md` (15 sections, milestones M1–M6, doc v1.9).
## Current Status

### Commits (`origin/main` at `d61c75c`; local HEAD = `b05c790`, NOT pushed — 1 commit ahead; 4 modified files uncommitted = GROUPED_BARS work)

> ⚠️ State correction (2026-08-01): previous save recorded local HEAD = `a0ac064` "7 commits ahead" — since then the user pushed 6 commits (`98d6e30`…`d61c75c`), and the scan-refactor commit hash changed `a0ac064` → `b05c790` (amend). Verify against `git rev-parse HEAD origin/main` — don't trust the table blindly (Lesson 22).

| Hash | Message |
|---|---|
| `b05c790` | ♻️ refactor(scan): replace ad-hoc special-case scopes with CustomEvaluator + shared helpers (**NOT pushed**) |
| `d61c75c` | ♻️ refactor: implement standard ESPRIT with FB averaging and adaptive L (origin/main) |
| `c9df40f` | 📝 docs: add ESPRIT optimization analysis and corrected Unitary ESPRIT flow |
| `49e32e9` | docs: update README and progress |
| `d408f7b` | ♻️ refactor(metrics): make name() a locale-independent identity key |
| `98d6e30` | ✨ feat(scan): rework test specs, localize panel, and pad charts |
| `a065e9f` | docs: add README and LICENSE (prior origin/main) |
| `778af51` | ✨ feat(scan): add per-peak error visualization for interference scan |
| `89a11e9` | 🐛 fix(ui): resolve LogPanel ring buffer OOB read causing SIGSEGV |
| `7c400f5` | ✨ feat(scan): implement batch scan test runner and results panel |
| ... | (complete log up to initial commit `d41ec19` — unchanged from prior state) |

### Milestone Progress

| # | Milestone | Status |
|---|---|---|
| M1 | Core types + Signal/Window skeleton + MonteCarlo complete | ✅ Done (`7875d71`) |
| M2 | FFT estimators + core/fft + rng + signal + window + metrics | ✅ Done (multiple commits) |
| M3 | MUSIC/ESPRIT | ✅ Done — MUSIC (beam-space via Eigen SVD); ESPRIT refactored to standard ESPRIT + FB averaging + adaptive L (real covariance, all-real arithmetic, LS normal equations). See OQ-33b for the Unitary ESPRIT abandonment rationale. |
| M4 | (merged into M2) Metrics + Statistics; **Batch Scan Tests** | ✅ Metrics/Stats done; ✅ Scan tests fully implemented and crash-free (7 tests / **14 charts** after v1.8 rework; was 23). LogPanel OOB fixed (`89a11e9`) |
| M5 | UI complete + main.cpp | ✅ Done (`bf920b2`) |
| M6 | End-to-end smoke test | ✅ Done — user verified all 4 algorithms |

**Refactor work outside milestones**:
- PeakFinder utility extracted from `findPeaksFromDft` (`388e990`). Doc tracked as v1.1 in `development_solution.md` (§5.5, OQ-8/9/10) but **not** as a formal milestone per user instruction.
- **i18n layer** (`de01c29` + `f422992`) — GNU gettext integration with two text domains (`ui`/`con`), Win32 locale auto-detection, zh_CN translation. Outside M1–M6 scope; user-initiated enhancement.
- **Metrics revision** (`85e330f`) — removed metric enable/disable checkbox; corrected RMSE (was computing MSE, name was `"MSE"`); added per-metric `format()` + `showDistribution()` to `IMetric`; `RunResult` changed from `unordered_map<string, MetricStats>` to `vector<MetricResult>`. Doc tracked as v1.2 in `development_solution.md` (§6.4, §8.3, §8.5, OQ-11/12/13). Outside M1–M6 scope; user-initiated revision.
- **UI bug fixes** (`a73421b` + `ad94f78`) — fixed double-shutdown segfault on window close (OQ-14); fixed ImPlot axis auto-refit on new experiment data via conditional `ImPlotCond` (OQ-15); added interference frequency reference line (OQ-16); spectrum plots wrapped in resizable `BeginChild` containers. Doc tracked as v1.3 in `development_solution.md` (§7.3, §8.4, §8.7, OQ-14~16).
- **Signal amplitude removal** (`f9f800d`) — removed `SignalSpec::Amplitude`; fixed to 1.0; interference amplitude now implicitly relative. Doc tracked as v1.4 (§5.2, §6.2, §8.3, OQ-17).
- **Metrics architecture redesign** (committed `b6c7efc` + `568de70`, doc v1.5 OQ-18~21) — three changes:
  1. `FrequencyPeak` gains `Prominence` field + `PROMINENCE_UNKNOWN` sentinel (OQ-18)
  2. `RmseMetric` → `MseMetric`: files renamed; `format()` no sqrt; max-Prominence peak selection (OQ-19+21)
  3. `RelativeEfficiencyMetric` added but **disabled** — model assumptions don't match CRB conditions; code retained but not wired (OQ-20)
- **Batch scan test architecture** (implemented, doc v1.6→v1.7 OQ-22~28) — `ScanTestRunner` wraps `ExperimentRunner` to scan parameters across ranges. Three dimension roles (X-axis / series / chart-split). Chart styles (LineWithErrorBands / GroupedBarsWithError / MultiLine; **+ GroupedBars added 2026-08-01, see OQ-35 entry below**). 7 concrete tests defined (originally 23 charts). New `ScanResultsPanel` with resizable ImPlot windows. Test 7 (Interference scan) uses special `PerPeak` mode: extracts all detected peaks per X-point, sorts by error distance, plots each rank as a separate series. LogPanel ring buffer OOB fixed (`89a11e9`).
- **Scan test rework + panel i18n + chart polish** (2026-07-30, **committed `98d6e30`, not pushed**; doc v1.7→v1.8 OQ-29~31) — three changes:
  1. **Test spec rework (OQ-29)**: Tests 1/2/4 → Algorithm as *series* (4 lines) + MULTI_LINE (drop error bands); Test 1 keeps both metrics (2 charts), Tests 2/4 = 1 each; Test 4 drops `GenerateOverview`. Test 5 → `ChartDim=Algorithm{Interpolate,MUSIC,ESPRIT}` (3 charts; adds MUSIC/ESPRIT). Test 6 → `ChartDim=SNR{−3,10 dB}` (2 charts; adds 10 dB), and fixed X-axis algo set to Interpolate/MUSIC/ESPRIT. Tests 3/7 unchanged. **Total 23→14 charts.** Consequence: `LineWithErrorBands` (Style A) is now unreferenced by any test (dead render path, retained).
  2. **Full scan-panel i18n (OQ-30)**: `ChartResult` gained atomic title fields `TestName`/`ChartDimLabel`/`IsOverview`; `SeriesResult` gained `PeakRank`. Worker stores English msgids only (no `_UI()` in `scan_test_runner.cpp` beyond the pre-existing metric-matching line); UI thread localizes via `localizedTitle()` / `localizedSeriesLabel()` (`"Peak %d"` → snprintf → "峰 N") / `_UI()`. Tick-label arrays materialized into `std::vector<std::string>` first to dodge the dgettext static-buffer aliasing trap. 19 new msgids added to `ui.po`/`ui.pot` (zh_CN). **Lesson 21 corrected**: metric `name()` actually returns `_UI(...)` (localized) and IS called on the worker thread — the "preventive fix" described in old Lesson 21 was never applied to the metric classes.
  3. **Chart padding + width (OQ-31)**: `ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.1,0.1))` around `render()` → 5% padding each side both axes (verified via `ApplyFit`: each side += `(range/2)×pad`); scoped to scan panel only. `plotSize()` narrows plot width by `fontSize×2.5` so the rightmost X label clears the child border. Removed the hardcoded grouped-bars `SetupAxisLimits(±0.6)`.
- **Metric identity-key refactor — worker-thread gettext eliminated** (2026-07-30, **committed `d408f7b`, not pushed**; doc v1.9 OQ-32) — `IMetric::name()` now returns the English msgid literal (no `_UI()`), making it the locale-independent identity key (gettext-canonical, DRY — `name()` *is* the id, no separate `id()` method). Scan matching → `name() == metric_name`; `experiment_runner` was already gettext-free; display layer's existing `_UI(name())` becomes a correct single translation. Result: **zero** `_UI`/`dgettext` on any worker thread; `src/metrics/` retains only `relative_efficiency::format()`'s `_UI("N/A")` (UI thread). Bonus: `name()`'s `string_view` now spans permanent literal storage, not gettext's static buffer. Implements the fix old Lesson 21 described but never actually applied.
- **ESPRIT optimization implementation (OQ-33→OQ-33b, 2026-07-31)** — Refactored `src/estimator/esprit.cpp` from standard complex ESPRIT to standard real-domain ESPRIT + FB averaging + adaptive L. During implementation, two critical bugs were discovered in `esprit.md` by independent LLM agent sessions:
  1. **Q-transform block formula (§3 Step 3)**: Produces block-diagonal T_X → signal subspace from covariance EVD only sees half the signal structure (bug in the mathematical shortcut, not in the original Haardt paper)
  2. **K₂ sparse index contradiction (§5.3 vs §6.2)**: Two contradictory formulas in the same document; §6.2 version creates linearly dependent rows in K₂Es
  Resolution: Abandoned the Q-transform + K₁/K₂ pipeline. Replaced with standard ESPRIT selection matrices on FB-averaged real covariance. Retained optimizations from the analysis: adaptive L (K=1→N/4), all-real arithmetic, normal-equation LS, skipping Vandermonde. The implementation may be OK at present — further optimization (Lanczos top-r EVD, Spectra) can be deferred.
- **Scan pipeline refactoring — CustomEvaluator + shared helpers** (2026-08-01, **committed `a0ac064`**, not pushed; doc v2.2, OQ-34) — Replaced the ad-hoc `ComputeTimeRatio` boolean flag with a `CustomEvaluator` std::function on `ScanTestDef`. Any future custom scan test now requires only a ~15-line lambda rather than a new boolean flag + inline scope. Per-peak mode extracted into `runPerPeakTest()` private method. Three shared helpers (`buildPointConfig`, `resolveEstimator`, `runExperimentSafe`) eliminate duplicated config/estimator/runner logic across all three code paths. `run()` reduced from ~610 to ~307 lines. Test 8 (Compute Time Ratio) reimplemented as a `CustomEval` lambda that runs `ExperimentRunner` twice per point (with/without interference) and returns the ratio.
- **GROUPED_BARS chart style (2026-08-01, 4 files modified, **NOT committed** — awaiting commit; doc update pending OQ-35)** — New `ChartStyle::GROUPED_BARS` = `GROUPED_BARS_WITH_ERROR` minus error whiskers (only mean bars). Changes: (1) enum entry in `scan_test_runner.h`; (2) `renderGroupedBars()` declaration in `scan_results_panel.h`; (3) implementation in `scan_results_panel.cpp` — copy of `renderGroupedBarsWithError()` without the `PlotErrorBars` block (~13 lines) + dispatch case in `renderChart()`; (4) Test 3 (NoiseDist × Algorithm) and Test 8 (Compute Time Ratio) switched to `GROUPED_BARS`; **Test 6 (Window × Algorithm) intentionally keeps `GROUPED_BARS_WITH_ERROR`**. Validated: clang-format ✅, build ✅ (5/5, no warnings), clang-tidy ✅ (4/4 files, no diagnostics). ⚠️ Lesson 25: the initial `replaceAll` on `t.Style = ChartStyle::GROUPED_BARS_WITH_ERROR;` also changed Test 6 and missed Test 8 (different preceding line) — both were fixed with targeted edits. Development_solution.md needs an OQ-35 entry (chart-style table §7.5.3 / §8.8 / OQ-24/29 mentions "three styles").

### Critical Bug Resolved: LogPanel Ring Buffer OOB (`89a11e9`)

**Symptom**: Multi-test scan crashed silently (SIGSEGV, signal 11) when tests 4–7 ran together. Crash was always at the same point (~200 log entries accumulated). Individual tests or small subsets ran fine. No C++ exception; try/catch couldn't catch it.

**Diagnosis path** (followed in this order, each ruled out):
1. **ImGui 16-bit vertex index limit** → Enabled 32-bit `ImDrawIdx` in `imconfig.h`. Still crashed. Ruled out.
2. **gettext thread-safety** → Worker thread called `_UI()` via `metric->name()`. Applied fix (metric `name()` returns literal English). Still crashed. Ruled out as root cause (but kept as preventive fix).
3. **Heap corruption from MUSIC/ESPRIT** → Built isolated repro (MUSIC 2100 + ESPRIT 2100 + FftInterpolate 100 iterations). All passed. Ruled out.
4. **`-march=native` AVX alignment** → Rebuilt without `-march=native`. Still crashed. Ruled out.
5. **Thread identification** → Added TID logging to crash handler. **TID=2850328247 = main thread**, not worker. The worker continued running after the crash.
6. **Per-render-call isolation** → Added stderr guards around every ImGui/GL call. Last line before crash was always `[main] Log start` with no `[main] Log done`.
7. **LogPanel ring buffer** → Disabling `Log.render()` eliminated the crash. Reducing `MAX_LOG` to 50 changed the crash to an ImGui assertion ("Forgot to call Render()"). The assertion revealed an exception escaped `Log.render()`, causing `ImGui::Render()` to be skipped.

**Root cause**: `LogPanel::render()` used `NextIdx` (which grows without bound) as the upper bound for iterating `Messages`. After the ring buffer wrapped (`NextIdx ≥ MAX_LOG`), the loop `for (i = 0; i < NextIdx; ++i)` read past `Messages.size()`, causing an out-of-bounds access → SIGSEGV on the main thread during rendering.

The crash appeared correlated with the worker thread's computation because it always happened after ~200 log entries (filling the buffer), which coincided with reaching test 5 in the scan sequence.

**Fix** (`89a11e9`):
1. Ring buffer iteration uses `NextIdx % MAX_LOG` when `Wrapped` is true — two-segment render (tail from `WRAP` to `MAX_LOG`, then head from `0` to `WRAP`).
2. Messages copied into a `RenderCopy` vector under the lock; ImGui renders from the copy. `RenderCopy` is cleared at the start of the next `render()` call (after `ImGui::Render()` has consumed the previous frame's pointers).
3. Added `RenderCopy` member to `LogPanel`.

### What's Implemented (code on disk, ALL COMPLETE)

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
- `src/estimator/esprit.cpp` — ✅ **Refactored (2026-07-31, OQ-33b)**. Standard ESPRIT + FB averaging + adaptive L on real covariance. All-real arithmetic (MatrixXd) for Hankel + FB averaging + covariance EVD; complex EVD only for tiny r×r Ψ (r ≤ 4). Retains adaptive L from esprit.md analysis (K=1→N/4, ~8× speedup). LS via normal equations. Unit-circle reliability check. Skips Vandermonde reconstruction. The Unitary ESPRIT Q-transform + K₁/K₂ pipeline from esprit.md was abandoned after discovering that the block formula produces a structurally block-diagonal T_X (corrupting signal subspace) and the K₂ formulas are contradictory/incorrect.

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

### 20. LogPanel ring buffer OOB read — unbounded index used as loop bound (CRITICAL, `89a11e9`)

**Mistake**: `LogPanel` uses a ring buffer where `NextIdx` increments without bound (it is the total write count, never reset). The `render()` method iterated `for (i = 0; i < NextIdx; ++i)` to display messages. Once the buffer wrapped (`NextIdx ≥ MAX_LOG = 200`), this loop read past `Messages.size()`, accessing unallocated memory → SIGSEGV on the main thread.

**Why it was hard to diagnose**: The crash manifested as a silent SIGSEGV (signal 11) that could not be caught by try/catch. It appeared correlated with the worker thread's computation (always at the same scan test point), leading to multiple wrong hypotheses (ImGui vertex limit, gettext thread-safety, heap corruption from Eigen, AVX alignment). The actual cause was a simple off-by-N read in the UI rendering path that only triggered after exactly `MAX_LOG` log entries accumulated.

**Diagnosis breakthrough**: Adding thread-ID to the crash handler proved the crash was on the **main thread** (not the worker). Adding per-render-call stderr guards proved the crash was inside `LogPanel::render()`. Disabling `LogPanel::render()` eliminated the crash entirely.

**Resolution**: Use `NextIdx % MAX_LOG` when `Wrapped` is true, rendering in two segments (oldest-to-newest chronological order). Additionally, copy messages into a `RenderCopy` vector under the lock before passing `c_str()` pointers to ImGui, preventing the worker thread from invalidating them before `ImGui::Render()`.

**Lesson**: Ring buffer cursors that grow without bound must **never** be used directly as array iteration bounds. Always apply `% capacity` when indexing into the backing array. When a crash appears correlated with background computation, verify which thread actually crashes before attributing it to the computation — a SIGSEGV in the rendering loop can look like a computation crash if the timing is consistent.

### 21. Worker-thread gettext calls — thread-safety hazard (preventive, not root cause)

**Mistake**: `IMetric::name()` implementations wrapped their return string in `_UI()` (→ `dgettext("ui", ...)`). The scan test worker thread called `metric->name()` during metric matching. GNU gettext's `dgettext` is not guaranteed thread-safe. Although this turned out **not** to be the root cause of the crash (the real cause was Lesson 20), it is still a correctness hazard.

**Resolution (as written 2026-07)**: `name()` returns a stable English msgid literal (no `_UI()`). The UI display layer translates on the main thread via `_UI(metric->name().data())`. The worker thread compares against English msgids directly — zero `dgettext` calls off the main thread. (This fix was applied during debugging and kept as a preventive measure.)

> ⚠️ **CORRECTION (2026-07-30, verified against code)**: The above Resolution was **never actually applied to the metric classes**. All four `IMetric::name()` implementations still `return _UI(...)` (localized) — `compute_time.cpp:14`, `percentage_error.cpp:27`, `mse.cpp:25`, `relative_efficiency.cpp:24`. So `name()` IS called on the worker thread (during scan metric matching) and DOES call `dgettext`. This is a pre-existing, accepted-in-practice hazard (no crash observed; the real Lesson-20 crash was the ring buffer). Consequence: scan metric matching MUST compare localized==localized (`name() == _UI(metric_name)`) to stay locale-independent — see Lesson 22. Fully removing worker-thread gettext requires giving metrics a locale-independent `id()` (separate task).
>
> ✅ **RESOLVED (2026-07-30, OQ-32)**: The Resolution is now applied for real — all four `IMetric::name()` return the English literal (no `_UI()`); scan matching uses `name() == metric_name`; worker-thread gettext is fully eliminated. Approach: `name()` itself is the locale-independent identity key (gettext-canonical, DRY — single source of truth) rather than a separate `id()` method; the display layer's existing `_UI(name())` call becomes a correct single translation. Bonus: `name()`'s `string_view` now points at permanent literal storage instead of gettext's static buffer.

**Lesson**: Any function callable from a background thread must NEVER call `dgettext`/`gettext`/`_UI()`. i18n is a display-only concern — do it on the UI thread at render time. Stable English msgids serve as locale-independent comparison keys. *(Caveat — see the correction above + Lesson 22: in this codebase the metric classes still violate this and it's accepted in practice. Verify reality, don't trust this file blindly.)*

### 22. Verify docs/lessons against actual code — old Lesson 21 was inaccurate (2026-07-30)

**Mistake**: While i18n'ing the scan panel, I trusted old Lesson 21's claim that `IMetric::name()` "returns a stable English msgid literal (no `_UI()`)" and diagnosed the scan metric-matching line `name() == _UI(metric_name)` as a "bug" (English `name()` vs localized `_UI()`). I "fixed" it to `name() == metric_name` (English==English). **That fix would have broken metric matching in zh_CN** — `name()` actually returns `_UI(...)` (localized), so the original localized==localized comparison was correct.

**Resolution**: Reverted to `name() == _UI(metric_name)` before it could ship; corrected Lesson 21 with a ⚠️ note; recorded as OQ-30.

**Lesson**: **Context/progress docs can be stale or aspirational.** Before acting on a claimed code state ("X returns English", "Y was fixed"), open the actual source and verify. A lesson's "Resolution" describes *intent*, not necessarily *reality* — especially for "preventive" fixes that may never have landed. When a fix rests on an assumption about existing behavior, verify that assumption first.

### 23. `replaceAll` can match code you just wrote — verify after every pass (2026-07-30)

**Mistake**: Used `replaceAll` to change every `_UI(s.Name.c_str())` → `localizedSeriesLabel(s).c_str()` in `scan_results_panel.cpp`. The pattern also matched the `return _UI(s.Name.c_str());` fallback **inside the newly-added `localizedSeriesLabel` helper itself**, turning it into infinite recursion. A follow-up edit (fixing an unrelated clang-tidy warning) then left a stray duplicate `return …; }` at namespace scope, breaking the file's brace structure.

**Resolution**: The build failed immediately (`ninja: subcommand failed`) and clang-tidy flagged the brace cascade; I located and removed the stray lines. No broken code reached a "done" state.

**Lesson**: `replaceAll` is global within the file — it matches occurrences inside helpers you added in the same pass, not just the intended call sites. After any `replaceAll`: (a) re-read the affected region; (b) lean on the build + clang-tidy gate, which catches the collateral instantly. When the replacement token also appears inside a helper that consumes it, prefer per-function scoped edits over `replaceAll`.

### 24. ImPlot `FitPadding` semantics — derive from source, not the doc blurb (2026-07-30)

**Risk avoided**: ImPlot's `FitPadding` doc says "ImVec2(0.1,0.1) adds 10% to the fit extents" — ambiguous (10% per side? 10% total?). The requirement was "5% each side".

**Resolution**: Read `ImPlotAxis::ApplyFit` in `implot_internal.h`: `FitExtents.Min/Max -= / += (FitExtents.Size()*0.5) * padding` — i.e. each side grows by `(range/2) * padding`. So padding=0.1 → each side += 5% of range. Confirmed `0.1` = exactly "5% each side" (and matches the doc's "+10% total").

**Lesson**: For library APIs with unit/percentage semantics, the one-line doc comment is often ambiguous. When a requirement is quantified ("5% each side"), open the implementation and derive the exact input rather than guessing from the prose. Same family as Lessons 11 & 22: **trust the source over the summary.**

### 25. `replaceAll` over-matches legitimate duplicates — verify WHICH sites changed (2026-08-01)

**Mistake**: Adding `ChartStyle::GROUPED_BARS`, I ran `replaceAll` on `t.Style = ChartStyle::GROUPED_BARS_WITH_ERROR;` → `GROUPED_BARS` in `scan_test_runner.cpp`, intending to change Tests 3 and 8 only. Three tests had that exact line (3, 6, 8), so `replaceAll` changed **all three** — including Test 6, which was supposed to keep error whiskers. Meanwhile Test 8's line did NOT match because its preceding line differed (`MetricNames = {"Compute Time Ratio"}` vs `{"Percentage Error"}`), so the edit *missed* the very test it was targeting. Both errors were caught by a follow-up `rg` verification and fixed with targeted edits (revert Test 6, change Test 8).

**Resolution**: After any `replaceAll`, list every changed site (`rg -n <pattern> <file>`) and check each one against intent — don't assume "all occurrences" == "the ones I meant". For sparse targeted changes where only some sites should change, **prefer unique-context single edits over `replaceAll`** (match includes the preceding `MetricNames` line or similar distinguishing context).

**Lesson**: `replaceAll` has two failure modes: (a) it hits *more* sites than intended (legitimate duplicates), and (b) it *misses* sites whose surrounding context differs from the match string. Related to Lesson 23 (replaceAll matched newly-written helper code) but distinct: Lesson 23 was about self-inflicted collateral, this one is about over/under-matching across pre-existing legitimate occurrences. Always verify the actual set of changed sites with `rg` after a bulk edit. The build + clang-tidy gate cannot catch a *wrong-but-valid* style choice (Test 6 with wrong style still compiles clean) — semantic verification requires reading the diff.

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

- `.opencode/context/development_solution.md` — authoritative plan (**v2.2**, ~1660 lines). Updated through: ...**v2.2 (2026-08-01) — scan pipeline refactoring**: replaced `ComputeTimeRatio` boolean flag with `CustomEvaluator` std::function; extracted `runPerPeakTest()` private method; extracted shared helpers (`buildPointConfig`/`resolveEstimator`/`runExperimentSafe`). `run()` reduced from ~610 to ~307 lines (OQ-34).
- `.opencode/context/progress.md` — this file
- `.opencode/context/esprit-1.md` — ESPRIT analysis agent #1 (aggressive L reduction; **has model-order error: r=K**)
- `.opencode/context/esprit-2.md` — ESPRIT analysis agent #2 (conservative L=N/2; **has K₁ formula error and internal freq-filtering violation**)
- `.opencode/context/esprit-3.md` — ESPRIT analysis agent #3 (correct model order r=2K; square-root SVD preferred)
- `.opencode/context/esprit-comparison.md` — cross-file audit and error diagnosis of esprit-1/2/3.md (written 2026-07-31)
- `.opencode/context/esprit.md` — **final deliverable**: corrected Unitary ESPRIT implementation guide (written 2026-07-31, v2.0)
- `.tmp/sessions/2026-07-30-scan-test-i18n-rework/context.md` — stale (committed in `98d6e30`; safe to delete)
- `.tmp/sessions/2026-07-19-m1-core-signal-montecarlo/context.md` — stale (M1 complete in `7875d71`; safe to delete)
- `.tmp/sessions/2026-07-31-esprit-opt-refactor/context.md` — stale (committed in `d61c75c`; safe to delete)
- `.tmp/sessions/2026-07-31-scan-pipeline-refactor/context.md` — session for this refactoring (committed in `a0ac064`; can delete after verification)
- `.tmp/sessions/2026-08-01-grouped-bars-style/context.md` — session for the GROUPED_BARS chart style (uncommitted; safe to delete after commit)
