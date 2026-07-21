# 信号频率估计仿真实验框架 — 开发计划

| 项目 | 内容 |
|---|---|
| 文档版本 | v1.0 |
| 制定日期 | 2026-07-19 |
| 状态 | 已确认，待实施 |
| 适用代码库 | `ISPPracticeOne`（C++20 / MSYS2 UCRT64） |

---

## 1. 项目目标

构建一个**单频正弦实数信号的频率估计仿真实验框架**，支持：

- 在 5 个相互正交的环境维度组合下，对单频正弦实数信号进行频率估计；
- 提供 4 种估计算法（FFT 直接峰值 / FFT 插值 / MUSIC / ESPRIT）；
- 通过蒙特卡洛仿真评估不同随机噪声对估计算法的影响；
- 多种评价指标 + 统计聚合（次数 > 1 时计算均值/标准差/极大/极小）；
- 通过 ImGui + ImPlot 实现 GUI 驱动，包含频谱图目测。

### 当前阶段范围

**5 个环境维度暂时仅作为用户配置项**（提供交互控件供用户调整），**不进行自动维度扫描**。蒙特卡洛仿真只覆盖"固定信号参数 + 固定环境配置"下不同随机噪声的影响。

> **未来扩展预留**：架构上保留把任意维度改造为"扫描维度"的余地（例如 SNR 从 -20 dB 到 30 dB 步进 5 dB）。届时只需扩展 `ExperimentConfig` 与 `ExperimentRunner`，各算法与评价指标无须改动。**本版不实现扫描，但接口设计不得阻止未来加入。**

---

## 2. 代码库基线

| 项 | 现状 |
|---|---|
| `src/main.cpp` | 6 行 stub（`Hello, world!`） |
| `include/` | 空目录（已通过 `target_include_directories` 接入主目标） |
| `test/` | 两个交互式 demo（`test_fft.cpp`、`test_implot.cpp`），不是单元测试；**不动** |
| `third_party/` | `imgui` / `implot`（STATIC）、`pocketfft`（INTERFACE）已就位 |
| Eigen | **由用户自行添加为 submodule 并接入 CMakeLists**（不在本计划范围内） |
| 命名/风格 | 严格遵循 `AGENTS.md` 与 `.clang-tidy`（见 §13） |

---

## 3. 总体架构（分层）

```
┌──────────────────────────────────────────────────────────┐
│ App 层      src/app         主入口、应用生命周期           │
├──────────────────────────────────────────────────────────┤
│ UI 层       src/ui          ImGui 面板 + ImPlot 频谱图     │
├──────────────────────────────────────────────────────────┤
│ Experiment src/experiment   配置 / 蒙特卡洛 / 统计聚合      │
├──────────────────────────────────────────────────────────┤
│ Metrics     src/metrics     评价指标                       │
│ Estimator   src/estimator   估计算法（统一接口）            │
│ Signal      src/signal      信号生成 / 加噪 / 加干扰        │
│ Window      src/window      窗函数                         │
├──────────────────────────────────────────────────────────┤
│ Core        include/ispp/core  类型 / 参数 / 工具          │
├──────────────────────────────────────────────────────────┤
│ 第三方      PocketFFT | Eigen（用户接入） | ImGui/ImPlot    │
└──────────────────────────────────────────────────────────┘
```

**依赖方向严格向下**；上层不得被下层引用。

---

## 4. 目录结构

```
intelligent-signal-processing-practice-1/
├── CMakeLists.txt              ← 根构建（注册子模块库 + ISPP_WIN32_GUI option）
├── include/
│   └── ispp/                   ← 项目公共头根（命名空间隔离）
│       ├── core/
│       │   ├── types.h             ← RealArray, ComplexArray, FrequencyPeak, EstimationResult
│       │   ├── parameters.h        ← SignalSpec / WindowSpec / NoiseSpec / InterferenceSpec / EnvSpec
│       │   ├── rng.h               ← 统一随机数生成器封装（支持种子复现）
│       │   └── fft.h               ← 公共 FFT 工具：computeDft / findPeaksFromDft
│       ├── window/
│       │   └── window.h            ← WindowKind enum + applyWindow()
│       ├── signal/
│       │   └── signal_generator.h  ← 合成输入信号
│       ├── estimator/
│       │   ├── estimator.h         ← IEstimator 接口
│       │   ├── fft_peak.h
│       │   ├── fft_interpolate.h
│       │   ├── music.h
│       │   └── esprit.h
│       ├── metrics/
│       │   ├── metric.h            ← IMetric 接口
│       │   ├── percentage_error.h
│       │   ├── rmse.h
│       │   └── compute_time.h
│       └── experiment/
│           ├── experiment_config.h ← 单一配置 + MonteCarloConfig
│           ├── experiment_runner.h ← 蒙特卡洛循环（完整实现）
│           └── statistics.h        ← mean/std/min/max（完整实现）
├── src/
│   ├── app/
│   │   └── main.cpp                ← GUI 启动（完整实现）
│   ├── core/
│   │   ├── rng.cpp
│   │   └── fft.cpp                 ← 公共 FFT 工具（完整实现）
│   ├── window/
│   │   └── window.cpp
│   ├── signal/
│   │   └── signal_generator.cpp
│   ├── estimator/
│   │   ├── fft_peak.cpp            ← 用户实现（可调用 core/fft）
│   │   ├── fft_interpolate.cpp     ← 用户实现
│   │   ├── music.cpp               ← 用户实现
│   │   └── esprit.cpp              ← 用户实现
│   ├── metrics/
│   │   ├── percentage_error.cpp
│   │   ├── rmse.cpp
│   │   └── compute_time.cpp
│   ├── experiment/
│   │   ├── experiment_runner.cpp   ← 完整实现
│   │   └── statistics.cpp          ← 完整实现
│   └── ui/
│       ├── ui_manager.h/.cpp       ← GLFW/ImGui/ImPlot 初始化 + 主循环 + DPI
│       ├── panels/
│       │   ├── config_panel.h/.cpp
│       │   ├── spectrum_panel.h/.cpp
│       │   ├── results_panel.h/.cpp
│       │   └── log_panel.h/.cpp
│       └── widgets/
│           └── enum_combo.h/.cpp   ← 强类型 enum ↔ ImGui::Combo 桥接
├── test/                            ← 保持现状，不改动
├── third_party/
│   ├── imgui/ implot/ pocketfft/   ← 已有
│   └── eigen/                      ← 用户自行添加
└── .opencode/context/
    └── development_plan.md         ← 本文件
```

---

## 5. 核心数据结构

> 代码块仅为接口草案，遵循 `.clang-tidy` 命名规则；具体实现按 §12 的代码生成约束处理。

### 5.1 `include/ispp/core/types.h`

```cpp
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ispp {

// 实数信号采样序列；不持有采样率（采样率由上下文传入）
using RealArray = std::vector<double>;

// 单个估计出的频率点
struct FrequencyPeak {
    double frequencyHz;
    double amplitude;
};

// 估计算法完整输出（含频率-幅度对 + 计时）。
// Peaks 由 IEstimator 填充，ComputeTimeSec 由 ExperimentRunner 外部注入。
struct EstimationResult {
    std::vector<FrequencyPeak> peaks;
    double computeTimeSec;   // 由 Runner 填充：含窗函数施加 + 估计的完整耗时
};

} // namespace ispp
```

### 5.2 `include/ispp/core/parameters.h`

```cpp
#pragma once

#include "ispp/core/types.h"

#include <cstdint>

namespace ispp {

// 原始单频正弦信号描述
struct SignalSpec {
    double sampleRateHz;     // 采样率
    std::size_t sampleCount; // 采样点数
    double frequencyHz;      // 原始信号频率
    double amplitude;        // 原始信号幅度（线性）
    double phaseRad;         // 原始信号初相位
};

enum class WindowKind {
    Rectangular,
    Hamming,
    Hann,
    Blackman,
};

struct WindowSpec {
    WindowKind kind;
};

enum class NoiseDistribution {
    Gaussian,
    Uniform,
    Laplacian,
    Impulse,        // 椒盐噪声
};

struct NoiseSpec {
    NoiseDistribution distribution;
    double snrDb;           // 信噪比 [dB]
};

// 噪声上下文摘要（供 estimator 接口使用）。
// 与 NoiseSpec 分离：NoiseSpec 是噪声生成配置，NoiseInfo 是估计上下文，
// 仅包含 estimator 需要知晓的最基本信息。
// 预留扩展：未来可按分布携带额外数值参数（方差、尺度等）。
struct NoiseInfo {
    NoiseDistribution distribution;
    double snrDb;           // 信噪比 [dB]
};

struct InterferenceSpec {
    double deltaBins;       // 与原始信号频率的差，单位为 bin；== 0 表示无干扰
    double amplitude;       // 干扰幅度（线性）
};

// 聚合 5 个正交维度
struct EnvSpec {
    WindowSpec window;
    NoiseSpec noise;
    InterferenceSpec interference;
};

} // namespace ispp
```

### 5.3 `include/ispp/core/rng.h`（**完整实现**）

```cpp
#pragma once

#include <cstdint>
#include <random>

namespace ispp {

// 统一 RNG 封装，支持种子复现（蒙特卡洛可重现）
class Rng {
public:
    explicit Rng(std::uint64_t seed);

    /// 高斯分布 ~ N(mean, stddev²)
    double normal(double mean, double stddev);
    /// 均匀分布 ~ U(lo, hi)
    double uniform(double lo, double hi);
    /// 拉普拉斯分布（双指数分布），阶数 scale
    double laplace(double mean, double scale);
    /// 脉冲噪声：以概率 p 返回 magnitude，否则返回 0
    double impulse(double p, double magnitude);

private:
    std::mt19937_64 Engine;
};

} // namespace ispp
```

### 5.4 `include/ispp/core/fft.h`（**完整实现**）

公共 FFT 工具，供各 estimator 与 ExperimentRunner 频谱可视化复用。
从实数序列到粗略峰值的两步流水线独立于具体估计算法。

```cpp
#pragma once

#include "ispp/core/types.h"

#include <cstddef>
#include <vector>

namespace ispp {

/// @brief 对实数序列做 r2c FFT，返回单边复数谱。
///
/// 输出长度为 N/2+1。幅度已按单边谱归一化（×2/N），
/// 使正弦信号峰值幅度近似等于时域幅度。
/// FFT 失败时返回空数组。
ComplexArray computeDft(const RealArray& input);

/// @brief 从复数 DFT 幅度谱中提取局部极大值峰值。
///
/// threshold_factor 为相对最大幅值的比率阈值；
/// bin_hz = sample_rate / N；最多返回 max_peak_count 个峰。
std::vector<FrequencyPeak>
findPeaksFromDft(const ComplexArray& dft, double threshold_factor,
                 double bin_hz, std::size_t max_peak_count);

} // namespace ispp
```

---

## 6. 模块设计

### 6.1 Window（骨架）

`include/ispp/window/window.h`

```cpp
#pragma once

#include "ispp/core/parameters.h"
#include "ispp/core/types.h"

namespace ispp {

// 对输入信号就地施加窗函数
void applyWindow(RealArray& signal, WindowKind kind);

} // namespace ispp
```

> 实现细节：矩形窗为 no-op；其余三种按标准系数计算。

### 6.2 Signal（骨架）

`include/ispp/signal/signal_generator.h`

```cpp
#pragma once

#include "ispp/core/parameters.h"
#include "ispp/core/rng.h"
#include "ispp/core/types.h"

namespace ispp {

class SignalGenerator {
public:
    /// @todo 合成"输入信号" = 原始正弦 + (可选)干扰 + 噪声
    ///   RealArray generate(const SignalSpec& signal,
    ///                      const EnvSpec& env,
    ///                      Rng& rng) const;
};

} // namespace ispp
```

**流水线**：纯正弦 → (+干扰，若 `deltaBins != 0`) → (+噪声，按分布与 SNR) → 输出实数信号。

### 6.3 Estimator（接口 + 4 个骨架；实现由用户完成）

`include/ispp/estimator/estimator.h`

所有与信号相关的上下文参数统一封装为 `EstimationContext`：

```cpp
/// 单次估计调用的上下文。
/// 封装 estimator 需要知晓但无法从 input 推导的信号信息。
struct EstimationContext {
    double sampleRateHz;        // 采样率 [Hz]
    WindowKind windowKind;      // input 上的窗函数类型（默认 Rectangular）
    std::size_t frequencyCount; // 信号频率分量数（无干扰 = MaxFreqCount，有干扰 = MaxFreqCount+1）
    NoiseInfo noiseInfo;        // 噪声分布 + 信噪比数值信息
};
```

```cpp
#pragma once

#include "ispp/core/parameters.h"
#include "ispp/core/types.h"

#include <string_view>
#include <vector>

namespace ispp {

class IEstimator {
public:
    virtual ~IEstimator() = default;

    // 输入：已加窗的实数信号 + 上下文（采样率/窗类型/频率数/噪声信息）
    // 返回频率-幅度对列表（不含计时；计时由 Runner 在外部完成）
    virtual std::vector<FrequencyPeak>
    estimate(const RealArray& input, const EstimationContext& context) = 0;
    virtual std::string_view name() const = 0;
};

} // namespace ispp
```

四个实现类。构造函数仅保留算法调优参数，
信号相关参数（频率数量、窗类型、噪声）通过 `EstimationContext` 传入：

```cpp
// fft_peak.h
class FftPeakEstimator : public IEstimator {
public:
    /// @param threshold 相对最大幅值的阈值因子（默认 0 = 不过滤）
    explicit FftPeakEstimator(double threshold = 0.0);
    std::vector<FrequencyPeak>
    estimate(const RealArray& input, const EstimationContext& context) override;
    std::string_view name() const override;
};

// fft_interpolate.h — 不同窗函数可能需要不同插值系数
class FftInterpolateEstimator : public IEstimator {
public:
    /// @param threshold 相对最大幅值的阈值因子
    explicit FftInterpolateEstimator(double threshold = 0.0);
    /// @todo 按 context.windowKind 选择插值算法；未知窗回退到通用插值
    std::vector<FrequencyPeak>
    estimate(const RealArray& input, const EstimationContext& context) override;
    std::string_view name() const override;
};

// music.h
class MusicEstimator : public IEstimator {
public:
    /// @todo 依赖 Eigen（用户接入后实现）；频率数由 context.frequencyCount 决定
    MusicEstimator() = default;
    std::vector<FrequencyPeak>
    estimate(const RealArray& input, const EstimationContext& context) override;
    std::string_view name() const override;
};

// esprit.h
class EspritEstimator : public IEstimator { /* 同 MUSIC */ };
```

| 算法 | 关键依赖 | 多频策略 | 上下文使用 |
|---|---|---|---|
| FFT 直接峰值 | PocketFFT（via `core/fft`） | 阈值 + 局部极大值 | 忽略 windowKind；frequencyCount 控制最多返回峰数 |
| FFT 插值 | PocketFFT（via `core/fft`） | 每峰值邻近 3 点抛物线/Quinn 插值 | **依赖** windowKind 选择插值系数；frequencyCount 控制峰数 |
| MUSIC | Eigen（SVD） | 由 `context.frequencyCount` 决定信号子空间维数 | 依赖 frequencyCount + noiseInfo |
| ESPRIT | Eigen（SVD + 子空间旋转） | 由 `context.frequencyCount` 决定 | 同上 |

> 单频是 `frequencyCount == 1` 的特例。
> 推荐流水线：`computeDft(input)` → `findPeaksFromDft(...)` →（可选）插值/子空间精修。

### 6.4 Metrics（接口 + 骨架）

`include/ispp/metrics/metric.h`

```cpp
#pragma once

#include "ispp/core/types.h"

namespace ispp {

class IMetric {
public:
    virtual ~IMetric() = default;

    /// @todo 单次评估：与真实频率比较
    ///   double evaluate(double trueFrequencyHz,
    ///                   const EstimationResult& result) = 0;
    virtual std::string_view name() const = 0;
};

} // namespace ispp
```

| 实现 | 行为 |
|---|---|
| `PercentageErrorMetric` | 与 `result.peaks` 中**误差最小的峰**比较（决策记录 OQ-6），返回 `%` |
| `RmseMetric` | 均方根误差 |
| `ComputeTimeMetric` | 直接返回 `result.computeTimeSec` |

---

## 7. Experiment / 蒙特卡洛（完整实现）

### 7.1 `include/ispp/experiment/statistics.h`（完整实现）

```cpp
#pragma once

#include <vector>

namespace ispp {

struct MetricStats {
    double mean;
    double std;
    double min;
    double max;
};

// 对一组样本计算算术均值/样本标准差/极大/极小；样本为空时返回全 0
MetricStats computeStats(const std::vector<double>& samples);

} // namespace ispp
```

### 7.2 `include/ispp/experiment/experiment_config.h`（骨架）

```cpp
#pragma once

#include "ispp/core/parameters.h"

#include <cstddef>

namespace ispp {

struct MonteCarloConfig {
    std::size_t iterationCount;   // 默认 100
    std::uint64_t baseSeed;       // 蒙特卡洛基准种子；第 i 次使用 baseSeed + i
};

struct ExperimentConfig {
    SignalSpec signal;
    EnvSpec env;
    MonteCarloConfig monteCarlo;
    std::size_t maxFreqCount;     // 用户配置：最大检测频率数（默认 1）。
                                  // 仿真假设只有单音信号，该字段与干扰维度共同
                                  // 决定 EstimationContext::frequencyCount：
                                  //   无干扰 → frequencyCount = maxFreqCount
                                  //   有干扰 → frequencyCount = maxFreqCount + 1
};

} // namespace ispp
```

### 7.3 `include/ispp/experiment/experiment_runner.h`（接口）

```cpp
#pragma once

#include "ispp/estimator/estimator.h"
#include "ispp/experiment/experiment_config.h"
#include "ispp/experiment/statistics.h"
#include "ispp/metrics/metric.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ispp {

// 单次实验完整运行结果
struct RunResult {
    // 每个 metric 名 → 统计聚合（次数 == 1 时 std/min/max 与 mean 相同）
    std::unordered_map<std::string, MetricStats> perMetricStats;

    // 末次迭代的可视化数据（供 UI 频谱面板使用）
    RealArray lastInputSignal;
    RealArray lastSpectrumFreqHz;     // 频谱横轴
    RealArray lastSpectrumMag;        // 频谱纵轴
    std::vector<FrequencyPeak> lastPeaks;
    double lastTrueFrequencyHz;

    // 整体运行耗时（含蒙特卡洛全程；用于显示，非评价指标）
    double totalRuntimeSec;
};

class ExperimentRunner {
public:
    using ProgressCallback = std::function<void(float /*[0,1]*/)>;

    ExperimentRunner(ExperimentConfig config,
                     std::shared_ptr<IEstimator> estimator,
                     std::vector<std::shared_ptr<IMetric>> metrics);

    // 同步运行；后台线程由 UI 调用方包装
    // onProgress 在每次蒙特卡洛迭代结束时回调（线程安全：仅 UI 端读取）
    RunResult run(ProgressCallback onProgress = nullptr);

    // 取消标志（UI 可在另一线程设置）
    void cancel();
    bool isCancelled() const;

private:
    ExperimentConfig config_;
    std::shared_ptr<IEstimator> estimator_;
    std::vector<std::shared_ptr<IMetric>> metrics_;
    std::atomic<bool> cancelled_{false};
};

} // namespace ispp
```

### 7.4 `ExperimentRunner::run()` 实现规约（**完整实现**）

```text
1. 预解析：binHz = sampleRate / sampleCount
2. 为每个 metric 准备 samples: vector<vector<double>>
3. RunResult result; result.lastTrueFrequencyHz = config.signal.frequencyHz;
4. for i in [0, iterationCount):
   a. 若 isCancelled() → break
   b. Rng rng(config.monteCarlo.baseSeed + i);
   c. RealArray input = SignalGenerator::generate(config.signal, config.env, rng);
   d. 计时开始（包含窗函数施加）
   e. applyWindow(input, config.env.window.kind);
    f. 构造 EstimationContext 并传给 estimator：

       ```text
       frequencyCount = config.monteCarlo.maxFreqCount +
                        (config.env.interference.deltaBins != 0 ? 1 : 0);
       NoiseInfo noiseInfo{config.env.noise.distribution, config.env.noise.snrDb};
       EstimationContext ctx{/*sampleRateHz*/ sampleRate,
                             /*windowKind*/   config.env.window.kind,
                             /*frequencyCount*/ frequencyCount,
                             /*noiseInfo*/     noiseInfo};
       auto peaks = estimator_->estimate(input, ctx);
       ```
    g. 计时结束 → 组装 EstimationResult{std::move(peaks), computeSec}
    h. for each metric: samples[m].push_back(metric->evaluate(trueFreq, er))
    i. 若 i == iterationCount - 1：缓存 lastInputSignal / lastSpectrum* / lastPeaks
       （频谱通过 computeDft(input) 计算，复用 core/fft）
    j. onProgress((i + 1) / iterationCount)
5. for each metric m: result.perMetricStats[m->name()] = computeStats(samples[m])
6. result.totalRuntimeSec = 全程计时
7. return result;
```

**关键不变量**：
- Runner 仅依赖 `IEstimator` 和 `IMetric` 接口；用户实现算法后**无需修改 Runner**。
- `iterationCount == 1` 时 `MetricStats` 四字段均等于该次单值。
- RNG 种子策略保证相同 `baseSeed` 下结果可复现。

---

## 8. UI 实现（完整实现）

### 8.1 启动与 DPI

参照 `test/test_implot.cpp` 的 DPI 处理方式：

| 项 | 实现 |
|---|---|
| 错误回调 | `glfwSetErrorCallback` |
| OpenGL | GL 3.0 + GLSL 130 |
| DPI 提示 | `glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE)` |
| 窗口尺寸 | `glfwGetPrimaryMonitor()` + `glfwGetVideoMode()` → 宽高 × 0.85 |
| DPI 读取 | `glfwGetWindowContentScale(window, &xscale, &yscale)` |
| ImGui 风格 | `ImGui::StyleColorsDark()` + `StyleAllSizes(xscale)` |
| 字体 | `AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 14.0f * xscale)` |
| 渲染适配 | `glfwGetFramebufferSize` + `glViewport`（同 test_implot） |
| 文本 | 简体中文硬编码，无 i18n |

> **字体路径硬编码为** `C:\Windows\Fonts\msyh.ttc`（Microsoft YaHei）。逻辑字号 14，按 DPI 物理放大。

### 8.2 UI 模块

```
src/ui/
├── ui_manager.h/.cpp       ← GLFW/ImGui/ImPlot 初始化 + 主循环 + DPI + 字体
├── panels/
│   ├── config_panel.h/.cpp ← 实验配置控件
│   ├── spectrum_panel.h/.cpp ← ImPlot 频谱目测
│   ├── results_panel.h/.cpp ← 指标表格 + 统计
│   └── log_panel.h/.cpp    ← 替代控制台
└── widgets/
    └── enum_combo.h/.cpp   ← 强类型 enum ↔ ImGui::Combo 桥接
```

### 8.3 配置面板交互映射

| 维度 | 控件 | 范围 |
|---|---|---|
| 窗函数 | `ImGui::Combo` | Rectangular / Hamming / Hann / Blackman |
| 频率偏移 | `ImGui::SliderFloat` | 0.0 ~ 0.5 bin（两端包含） |
| 信噪比 SNR | `ImGui::SliderFloat` | -20 ~ 30 dB（两端包含） |
| 噪声分布 | `ImGui::Combo` | Gaussian / Uniform / Laplacian / Impulse |
| 干扰 Δfreq | `ImGui::SliderFloat` | -2 ~ 2 bin（两端包含；== 0 自动判定无干扰） |

**附加控件**：
- 信号基础参数（采样率、采样点数、原始频率、幅度、相位）
- `maxFreqCount`（用于 MUSIC/ESPRIT）
- 蒙特卡洛次数（默认 100）
- 基准种子（可编辑）
- 算法选择（4 选 1，单选）
- 评价指标多选（勾选要启用的指标）
- **"运行" 按钮**：触发后台 `ExperimentRunner::run()`

### 8.4 频谱面板（ImPlot）

- 时域输入信号折线图（末次迭代 `lastInputSignal`）
- 加窗后单边幅度谱（横轴 `lastSpectrumFreqHz`，纵轴 `lastSpectrumMag`，提供 dB/线性切换）
- 估计峰值叠加散点（`ImPlot::PlotScatter`）
- 真实频率参考竖线（`ImPlot::PlotInfLines`）

### 8.5 结果面板

- 表格：每行一个 metric，列为 `mean / std / min / max`
- `iterationCount == 1` 时仅显示单值列（其他列隐藏或显示同值）
- 末次 `computeTimeSec` 单独标注

### 8.6 日志面板

- 应用内 `std::string` 消息缓冲（环形缓冲，最近 N 条）
- 滚动到底；可清空
- **替代被 `WIN32_EXECUTABLE TRUE` 关闭的 console**

### 8.7 主循环（`ui_manager.cpp`）

```text
初始化 GLFW/ImGui/ImPlot + DPI + 字体
while (!glfwWindowShouldClose(window)):
    glfwPollEvents()
    ImGui_ImplOpenGL3_NewFrame()
    ImGui_ImplGlfw_NewFrame()
    ImGui::NewFrame()

    可选：ImGui::DockSpace（启用 docking 时）

    configPanel.render(sharedConfig, runRequest)
    spectrumPanel.render(lastRunResult)
    resultsPanel.render(lastRunResult)
    logPanel.render()

    若 runRequest 为真 且 后台无任务:
        启动 std::thread 执行 ExperimentRunner::run(progressCb)
        UI 端用 progressCb 更新进度条
    若后台完成 → 拷贝 RunResult 到 lastRunResult

    ImGui::Render()
    glfwGetFramebufferSize(window, &w, &h)
    glViewport(0,0,w,h)
    glClearColor(...)
    glClear(...)
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData())
    glfwSwapBuffers(window)

清理 ImGui/ImPlot/GLFW
```

> **线程约定**：`ExperimentRunner::run()` 在独立 `std::thread` 中执行；UI 端仅在主线程读写 `RunResult`。`ProgressCallback` 通过原子或主循环 polling 传递进度。

---

## 9. main.cpp（完整实现）

`src/app/main.cpp` 调用 `ispp::ui::UiManager`（或等价入口）启动 GUI；不再保留 stub。

---

## 10. CMake 变更

### 10.1 新增 option（控制 console）

```cmake
option(ISPP_WIN32_GUI "Build as windowed GUI app (no console)" OFF)
```

主目标属性：

```cmake
if(ISPP_WIN32_GUI)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
```

- **开发期默认 OFF**（保留 console 便于排错）
- **发布期** `-DISPP_WIN32_GUI=ON`

### 10.2 新增源文件注册

将 `src/app`、`src/core`、`src/window`、`src/signal`、`src/estimator`、`src/metrics`、`src/experiment`、`src/ui` 各模块的 `.cpp` 添加到 `ISPPracticeOne` 目标的源列表；`include/` 已通过 `target_include_directories(... include)` 接入。

### 10.3 Eigen 接入

**由用户自行完成**（添加 `third_party/eigen` submodule 并在 CMake 注册为 INTERFACE 库）。MUSIC/ESPRIT 的 `.cpp` 在用户接入 Eigen 后再实现具体算法逻辑。

---

## 11. 开发里程碑

| # | 里程碑 | 实现程度 |
|---|---|---|
| M1 | Core 类型 + Signal/Window 骨架 + 蒙特卡洛完整 | 骨架 + MonteCarlo 实现 |
| M2 | FFT 估计器 + core/fft 公共工具 + WindowKind 参数 | 工具完整；fft_peak 完整；fft_interpolate 骨架 |
| M3 | MUSIC/ESPRIT 骨架 | 骨架 |
| M4 | Metrics 骨架 + Statistics 完整 | 骨架 + Stats 实现 |
| M5 | UI 完整 + main.cpp | **完整实现** |
| M6 | 端到端冒烟（用户实现任一算法后即可跑） | 验证 |

**每个里程碑完成后的强制步骤**（遵循 `AGENTS.md`）：
1. 对所有改动文件运行 `clang-format`
2. 对所有改动文件运行 `clang-tidy` 并清零诊断
3. 手动 GUI 冒烟（不跑 ctest、不写单元测试）

---

## 12. 代码生成约束

### 12.1 默认：仅骨架

**除 §7（蒙特卡洛/统计）、§8（UI）、§9（main.cpp）外，所有模块仅生成代码骨架**：
- `namespace` 完整
- `class` / `struct` 完整成员声明与接口签名
- 函数签名完整（参数/返回值/`const`/`noexcept`/`override`）
- 实现体使用 `/// @todo ...` 占位，不编写实际逻辑

### 12.2 例外：完整实现的部分

| 部分 | 完整实现范围 |
|---|---|
| `core/fft.{h,cpp}` | 全部（公共 FFT 工具：computeDft / findPeaksFromDft） |
| `core/rng.{h,cpp}` | 全部（四分布抽样：normal / uniform / laplace / impulse） |
| `experiment/statistics.{h,cpp}` | 全部 |
| `experiment/experiment_runner.{h,cpp}` | 全部（按 §7.4 规约） |
| `src/ui/**` | 全部（含 DPI、字体、主循环、所有面板与控件） |
| `src/app/main.cpp` | 全部（GUI 启动） |
| `CMakeLists.txt` 改动 | 全部（option、源文件注册） |

### 12.3 用户自行完成的部分

| 部分 | 责任方 |
|---|---|
| `window/window.cpp` 各窗函数系数 | 用户 |
| `signal/signal_generator.cpp` 合成流水线 | 用户 |
| `estimator/fft_peak.cpp` 峰值估计（可调用 core/fft） | 用户（可已完成） |
| `estimator/fft_interpolate.cpp` 插值估计（按 windowKind 分支） | 用户 |
| `estimator/music.cpp` / `esprit.cpp` | 用户 |
| `metrics/*.cpp` 各 evaluate 逻辑 | 用户 |
| Eigen submodule + CMake 接入 | 用户 |

> **算法实现完毕后无需修改蒙特卡洛或 UI**：因 `ExperimentRunner` 仅依赖 `IEstimator` / `IMetric` 抽象接口。
> **推荐**：estimator 内部优先调用 `computeDft` / `findPeaksFromDft`，避免重复实现 PocketFFT 封装。

---

## 13. 命名与风格

严格遵循 `AGENTS.md` 与 `.clang-tidy`，本文件不再重复贴细则。要点：

- C++20 strict（`CMAKE_CXX_EXTENSIONS=OFF`）
- GCC/Clang 警告：`-Wall -Wextra -Wpedantic -Wconversion -Wshadow`
- 命名：`UPPER_CASE`（常量）、`CamelCase`（类/枚举/联合/成员）、`camelBack`（函数）、`lower_case`（参数/变量）
- clang-format：LLVM base，4 空格缩进
- clangd 是语言服务器（不是 IntelliSense）

---

## 14. 开放问题决策记录

| ID | 议题 | 决策 |
|---|---|---|
| OQ-1 | Eigen 依赖 | 由用户自行添加为 submodule 并接入 CMake |
| OQ-2 | MUSIC/ESPRIT 多频数量 | 由用户在 UI 配置 `maxFreqCount`（默认 1）；通过 `EstimationContext::frequencyCount` 传递——无干扰时 =`maxFreqCount`，有干扰时 =`maxFreqCount+1` |
| OQ-3 | 维度扫描 | **本版仅作为用户配置项**（频率位置/SNR 用滑块，其余用 Combo）；蒙特卡洛只仿真固定信号+固定环境配置下不同随机噪声的影响；**未来可能扩展为维度扫描** |
| OQ-4 | 结果持久化 | **仅 GUI 显示**，不导出 CSV/JSON |
| OQ-5 | 算法时间测量边界 | **计入窗函数施加时间**（窗属于信号预处理） |
| OQ-6 | 多峰误差匹配 | 与 `peaks` 中**误差最小的峰**比较 |
| OQ-7 | `WIN32_EXECUTABLE` | 用 CMake option `ISPP_WIN32_GUI` 控制；开发期默认 OFF（保留 console），发布期 ON |

---

## 15. 未来扩展点（非本版实现）

- **维度扫描**：把 5 个维度任意子集改造为 sweep 列表；`ExperimentRunner` 内层增加 sweep 嵌套循环，外层仍为蒙特卡洛。`ExperimentConfig` 应预留扩展形态。
- **结果导出**：CSV/JSON。
- **多算法对比**：同一配置下并列运行多种算法，结果面板表格按算法分行。
- **国际化（i18n）**：当前 UI 文本硬编码简体中文；如需 i18n，需要先抽出字符串表。
