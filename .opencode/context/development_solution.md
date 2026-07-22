# 信号频率估计仿真实验框架 — 开发计划

| 项目 | 内容 |
|---|---|
| 文档版本 | v1.4 |
| 制定日期 | 2026-07-19 |
| 最近修订 | 2026-07-22（移除信号幅度参数，固定为 1.0；干扰幅度改为隐式相对值。见 §5.2 / §6.2 / §8.3 / OQ-17） |
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
| `src/main.cpp` | 调用 `UiManager::run()` 启动 GUI |
| `include/` | 已通过 `target_include_directories` 接入主目标 |
| `test/` | 两个交互式 demo（`test_fft.cpp`、`test_implot.cpp`），不是单元测试；**不动** |
| `third_party/` | `imgui` / `implot`（STATIC）、`pocketfft`（INTERFACE）已就位 |
| Eigen | **已接入**（`third_party/eigen` submodule + CMake INTERFACE target） |
| 构建配置 | Debug 模式额外添加 `-g` 编译选项（用户已在 CMakeLists.txt 中添加） |
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
│ Core        include/ispp/core  类型 / 参数 / 工具（含 PeakFinder）│
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
│       │   ├── fft.h               ← 公共 FFT 工具：computeDft / findPeaksFromDft
│       │   ├── peak_finder.h       ← 通用一维寻峰模板工具（PeakFinder<T>），fft / estimator 共用
│       │   └── peak_finder.tpp     ← PeakFinder 模板实现（由 .h 末尾包含，不参与 target_sources）
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
    └── development_solution.md     ← 本文件
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
    double phaseRad;         // 原始信号初相位
    // 幅度固定为 1.0（OQ-17）；干扰等其余源的幅度隐式相对此基准
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
    double amplitude;       // 干扰幅度（相对于信号幅度 1.0 的线性比值）
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
/// 实现委托给 `PeakFinder<double>::findPeaks`（见 §5.5）：
///   1. 计算 `|dft[i]|` 得到线性幅度数组 `mags`；
///   2. 调用 `PeakFinder<double>::findPeaks(mags, kernel_size, margin,
///      min_prominence, min_width)`；
///   3. 将返回的 `Peak::Index` 映射为
///      `FrequencyPeak{ .FrequencyHz = idx * bin_hz,
///                      .Amplitude   = mags[idx] }`；
///   4. 若返回峰数 > `max_peak_count`，按 `Prominence` 降序截取前 N 个。
///
/// **旧参数 → 新参数映射**（仅供实现参考，签名保持稳定）：
///   - `threshold_factor` → 内部转换为 `margin` 与 `min_prominence`
///     （相对全局最大幅度的工程合理默认值）；
///   - `max_peak_count` → 后置截断步骤（按 `Prominence` 降序）。
///
/// @warning 公开签名保持不变，所有现有调用方
/// （`FftPeakEstimator` / `FftInterpolateEstimator` /
/// `ExperimentRunner` 频谱快照）零改动。需要精细控制
/// (kernel/margin/prominence/width) 的调用方应直接使用 `PeakFinder`。
///
/// @param dft 单边复数 DFT（通常来自 computeDft）。
/// @param threshold_factor 相对最大幅值的比率阈值，范围建议 [0, 1]。
/// @param bin_hz 频率分辨率 Hz/bin（= sample_rate / N）。
/// @param max_peak_count 最多返回的峰值个数。
/// @return 频率-幅度对列表；dft 为空时返回空列表。
std::vector<FrequencyPeak>
findPeaksFromDft(const ComplexArray& dft, double threshold_factor,
                 double bin_hz, std::size_t max_peak_count);

} // namespace ispp
```

### 5.5 `include/ispp/core/peak_finder.h`（**完整实现**）

通用一维数据寻峰工具，与具体物理量（频率/幅度/相位）解耦。除供 §5.4
`findPeaksFromDft` 委托外，MUSIC/ESPRIT 的伪谱寻峰（§6.3）以及未来
维度扫描可视化均可直接复用，无需重复实现寻峰逻辑。

#### 5.5.1 接口签名

```cpp
#pragma once

#include <concepts>
#include <cstddef>
#include <span>
#include <vector>

namespace ispp {

template <std::floating_point T>
class PeakFinder {
public:
    /// 寻峰结果。仅承载算法输出（下标 + 突出度），
    /// 调用方可按需通过 `data[peak.Index]` 取回幅值。
    struct Peak {
        std::size_t Index;
        T Prominence;
    };

    /// @brief 寻找峰值的唯一公开接口。
    ///
    /// 流水线（详见 §5.5.3）：
    ///   中值滤波底噪 → 候选局部极大值 → margin 阈值
    ///   → prominence ≥ min_prominence → FWHM ≥ min_width
    ///
    /// @param data           输入的一维线性连续数据视图。
    /// @param kernel_size    中值滤波窗口大小（建议奇数，如 31）。
    /// @param margin         加在底噪（中值滤波结果）之上的固定安全裕度
    ///                       （线性阈值偏移）。
    /// @param min_prominence 峰值最小突出度。
    /// @param min_width      主瓣最小半高全宽（FWHM），默认 1.0。
    /// @return 满足全部条件的峰值数组，按下标升序。
    static std::vector<Peak> findPeaks(std::span<const T> data,
                                       std::size_t kernel_size,
                                       T margin,
                                       T min_prominence,
                                       T min_width = static_cast<T>(1.0));

private:
    // 滑窗中值滤波，作为底噪估计；边界处窗口收缩（不补零）。
    static std::vector<T> calcMedianFilter(std::span<const T> data,
                                           std::size_t kernel_size);

    // 计算峰值突出度（topographic prominence）：
    // 左右各向外搜索至鞍点，取两侧较高包络线之差。
    static T calcProminence(std::span<const T> data,
                            std::size_t peak_idx);

    // 计算 FWHM：以 prominence 半高做左右交叉点搜索，
    // 使用局部线性插值获得亚下标精度；返回宽度（单位为下标间距）。
    static T calcWidth(std::span<const T> data,
                       std::size_t peak_idx,
                       T prominence);
};

} // namespace ispp

#include "ispp/core/peak_finder.tpp"
```

> **签名说明**（相对参考签名的两处 C++ 合规修正）：
> 1. 移除 `static ... findPeaks(...) const` 末尾的 `const` ——
>    静态成员函数不可带 cv 限定符（参考签名存在该错误）。
> 2. 私有辅助函数 `calcMedianFilter` / `calcProminence` / `calcWidth`
>    统一声明为 `static` —— 与公开接口一致；类无任何实例状态，
>    所有方法均为输入数据的纯函数。
>
> **命名**严格遵循 `.clang-tidy`：
> - 成员 / 嵌套类型 → `CamelCase`（`Peak`、`Index`、`Prominence`）
> - 函数 → `camelBack`（`findPeaks`、`calcMedianFilter`、`calcProminence`、`calcWidth`）
> - 模板参数 → `CamelCase` 单字母（`T`）
> - 参数 / 局部变量 → `lower_case`（`data`、`kernel_size`、`peak_idx`）

#### 5.5.2 模板文件组织

模板实现放在 `include/ispp/core/peak_finder.tpp`，由 `peak_finder.h` 在
关闭 `namespace ispp` 之后、`#endif` 之前通过 `#include` 引入
（见上方签名末尾）。该 `.tpp` 文件**不参与 `target_sources`**，仅作头文件
片段被传递包含；保持 header-only 模板语义，避免显式实例化的翻译单元耦合。
建议在 `.clang-format` 与 `.clang-tidy` 中将 `peak_finder.tpp` 视作 C++ 头文件。

#### 5.5.3 算法流水线

```text
findPeaks(data, kernel_size, margin, min_prominence, min_width):
  1. baseline = calcMedianFilter(data, kernel_size)
  2. candidates = [ i | 0 < i < n-1,
                       data[i] > data[i-1] && data[i] > data[i+1],
                       data[i] - baseline[i] >= margin ]
  3. for each i in candidates:
       p = calcProminence(data, i)
       if p < min_prominence: skip
       w = calcWidth(data, i, p)
       if w < min_width: skip
       emit Peak{ .Index = i, .Prominence = p }
  4. return emitted peaks, sorted by Index ascending
```

- **中值滤波**对尖峰扰动具有鲁棒性，提供平滑的底噪参考；
  边界处窗口自动收缩以避免越界。
- **Prominence** 采用拓扑定义（左右各向外搜索至第一个高于当前峰的参考，
  之间取最低鞍点；prominence = data[peak] - max(left_saddle, right_saddle)），
  比单纯"高出底噪多少"更能区分真实主瓣与旁瓣。
- **FWHM** 使用局部线性插值获得亚下标精度，避免主瓣被误判为过窄
  （`min_width = 1.0` 默认值即"至少 1 个 bin 宽"）。

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

### 6.2 Signal（**完整实现**）

`include/ispp/signal/signal_generator.h`

```cpp
#pragma once

#include "ispp/core/parameters.h"
#include "ispp/core/rng.h"
#include "ispp/core/types.h"

namespace ispp {

class SignalGenerator {
public:
    /// 合成"输入信号" = 原始正弦 → (+干扰，若 deltaBins != 0) → (+噪声，按分布与 SNR)
    RealArray generate(const SignalSpec& signal, const EnvSpec& env,
                       Rng& rng) const;
};

} // namespace ispp
```

**流水线**：纯正弦（幅度 1.0）→ (+干扰，若 `deltaBins != 0`) → (+噪声，按分布与 SNR) → 输出实数信号。

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
>
> **MUSIC / ESPRIT 的伪谱寻峰**：在伪谱（pseudospectrum）数组生成后，
> 应直接调用 `PeakFinder<double>::findPeaks(pseudospectrum, ...)`（§5.5），
> 而非把伪谱塞进 `findPeaksFromDft` —— `findPeaksFromDft` 内部会做
> `|dft[i]|` 复数取模，对已经是线性实数的伪谱是错误的额外步骤。
> 直接使用 `PeakFinder` 可获得完整的 (kernel_size / margin /
> min_prominence / min_width) 调参能力。

### 6.4 Metrics（**完整实现**）

`include/ispp/metrics/metric.h`

```cpp
#pragma once

#include "ispp/core/types.h"

#include <string>
#include <string_view>

namespace ispp {

class IMetric {
public:
    virtual ~IMetric() = default;

    /// 单次评估：与真实频率比较，返回指标值
    /// 多峰时选取误差最小的峰（决策记录 OQ-6）
    virtual double evaluate(double trueFrequencyHz,
                            const EstimationResult& result) = 0;
    virtual std::string_view name() const = 0;

    /// 将统计值格式化为人类可读字符串（决策记录 OQ-13）。
    /// 各指标格式不同：百分比误差保留三位小数并加 `%` 后缀；
    /// RMSE 对 MSE 开根号后以 `%.6e` 显示；计算时间用 SI 单位 + 3 位有效数字。
    virtual std::string format(double value) const = 0;

    /// 是否在结果面板展示完整的统计分布（mean/std/min/max）。
    /// RMSE 返回 false（仅显示单一 RMSE 值），其余返回 true。
    virtual bool showDistribution() const { return true; }
};

} // namespace ispp
```

| 实现 | 行为 | 返回值（单次） | 结果面板展示 | `format()` 规则 |
|---|---|---|---|---|
| `PercentageErrorMetric` | 与 `result.peaks` 中**误差最小的峰**比较（OQ-6），返回百分比 | `\|Δf\| / f_true × 100%` | `showDistribution() = true`：完整统计列 | 保留三位小数 + `%` 后缀，如 `0.150%` / `12.345%` |
| `RmseMetric` | 均方根误差：单次返回 `(Δf)²`；MC 聚合后 `mean = MSE`，`sqrt(mean)` = RMSE；`name()` 返回 `"RMSE"`（原 `"MSE"`） | `(Δf)²` | `showDistribution() = false`：**仅显示单一 RMSE 值**；不展示 mean/std/min/max（对这些值求均值和标准差不具有数学意义） | 对 MSE 开根号后以 `%.6e` 显示（如 `1.234e-06`） |
| `ComputeTimeMetric` | 直接返回 `result.computeTimeSec` | `computeTimeSec` | `showDistribution() = true`：完整统计列 | SI 单位 + 3 位有效数字，如 `375ns` / `5.52us` / `10.5ms` / `1.23s` |

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
#include <vector>

namespace ispp {

// 单指标运行结果
struct MetricResult {
    std::shared_ptr<IMetric> MetricObj;
    MetricStats Stats;
};

// 单次实验完整运行结果
struct RunResult {
    // 每个指标的结果（按注册顺序；MetricObj 提供 format() 与 showDistribution()）
    std::vector<MetricResult> Metrics;

    // 末次迭代的可视化数据（供 UI 频谱面板使用）
    RealArray LastInputSignal;
    RealArray LastSpectrumFreqHz;     // 频谱横轴
    RealArray LastSpectrumMag;        // 频谱纵轴
    std::vector<FrequencyPeak> LastPeaks;
    double LastTrueFrequencyHz;
    double LastInterferenceDeltaHz;   // 干扰频偏（OQ-16）

    // 整体运行耗时（含蒙特卡洛全程；用于显示，非评价指标）
    double TotalRuntimeSec;
};

class ExperimentRunner {
public:
    using ProgressCallback = std::function<void(float /*[0,1]*/)>;

    ExperimentRunner(ExperimentConfig config,
                     std::shared_ptr<IEstimator> estimator,
                     std::vector<std::shared_ptr<IMetric>> metrics);

    // 同步运行；后台线程由 UI 调用方包装
    RunResult run(const ProgressCallback &onProgress = nullptr);

    // 取消标志（UI 可在另一线程设置）
    void cancel();
    bool isCancelled() const;

private:
    ExperimentConfig Config;
    std::shared_ptr<IEstimator> Estimator;
    std::vector<std::shared_ptr<IMetric>> Metrics;
    std::atomic<bool> Cancelled{false};
};

} // namespace ispp
```

### 7.4 `ExperimentRunner::run()` 实现规约（**完整实现**）

```text
1. 预解析：binHz = sampleRate / sampleCount
2. 为每个 metric 准备 samples: vector<vector<double>>
3. RunResult result; result.LastTrueFrequencyHz = config.signal.frequencyHz;
   result.LastInterferenceDeltaHz = config.env.interference.deltaBins * binHz;
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
5. for each metric m: result.Metrics.push_back({m, computeStats(samples[m])})
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
| 窗口尺寸 | `glfwGetMonitorContentScale` 获取物理→逻辑像素比例；`glfwGetVideoMode` 宽高 ÷ 缩放比 × 0.85 |
| DPI 读取 | `glfwGetWindowContentScale(window, &xscale, &yscale)` |
| ImGui 风格 | `ImGui::StyleColorsDark()` + `StyleAllSizes(xscale)` |
| 字体 | `AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 14.0f * xscale)` |
| 渲染适配 | `glfwGetFramebufferSize` + `glViewport`（同 test_implot） |
| 文本 | 简体中文硬编码，无 i18n |

> **DPI 归一化说明**：`glfwGetVideoMode` 返回物理分辨率（如 3840×2160），
> 需先通过 `glfwGetMonitorContentScale` 获取缩放比并除以缩放比得到逻辑像素，
> 再按 85% 计算窗口尺寸，避免在高 DPI 显示器上窗口超出屏幕。

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
- 信号基础参数（采样率、采样点数、原始频率、相位；幅度固定为 1.0 不暴露）
- `maxFreqCount`（用于 MUSIC/ESPRIT）
- 蒙特卡洛次数（默认 100）
- 基准种子（可编辑）
- 算法选择（4 选 1，单选）
- 评价指标：**全部始终启用**（不再提供勾选；移除 MetricsMask / Metrics 多选区域；`ExperimentRunner` 始终注册三个指标：PercentageError、RMSE、ComputeTime）
- **"运行" 按钮**：触发后台 `ExperimentRunner::run()`

### 8.4 频谱面板（ImPlot）

**布局**：波形与频谱各自包裹在 `ImGui::BeginChild(..., ImGuiChildFlags_ResizeY)` 容器中，用户可拖拽调整两个图的高度。容器宽度填满可用区域。

**轴范围自动重拟合**（OQ-15）：`SetupAxisLimits` 默认使用 `ImPlotCond_Once`（仅首帧生效），导致首次实验后轴范围冻结。修复策略：`SpectrumPanel` 记录 `LastInputSignalData` 指针，当 `UiManager` 移动赋值新 `RunResult` 时指针变化 → 该帧使用 `ImPlotCond_Always` 强制重设轴范围 → 后续帧恢复 `ImPlotCond_Once`（no-op），保留用户手动缩放。

| 图 | X 轴范围 | Y 轴范围 |
|---|---|---|
| 波形 | `[0, N-1]`（紧凑，无留白） | `[min, max] ± 8%` 数据范围 |
| 频谱 | `[freq.front(), freq.back()]`（紧凑，无留白） | `[min, max] ± 8%` 数据范围（dB） |

**绘制内容**：
- 时域输入信号折线图（末次迭代 `LastInputSignal`）
- 单边幅度谱 dB（`20*log10(mag)`，地板 -300 dB）
- 估计峰值叠加散点（`ImPlot::PlotScatter`）
- 真实频率参考竖线（`ImPlot::PlotInfLines`）
- 干扰频率参考竖线（当 `LastInterferenceDeltaHz ≠ 0` 时绘制，OQ-16）

### 8.5 结果面板

- 遍历 `PerMetricStats`，按各 metric 的 `showDistribution()` 分两种展示：

  | `showDistribution()` | 展示方式 |
  |---|---|
  | `true`（PercentageError、ComputeTime） | 表格行：`Metric \| Mean \| Std \| Min \| Max`；每个统计值经 `metric.format()` 格式化后显示 |
  | `false`（RMSE） | 单行：`"RMSE: <formatted_value>"`；`formatted_value = metric.format(mean)`（对 MSE 开根号 → RMSE） |

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

清理 ImGui/ImPlot/GLFW（仅在 `~UiManager()` 析构中调用 `shutdown()`，不在 `run()` 末尾调用——避免双重 shutdown 崩溃，OQ-14）
```

> **线程约定**：`ExperimentRunner::run()` 在独立 `std::thread` 中执行；UI 端仅在主线程读写 `RunResult`。`ProgressCallback` 通过原子或主循环 polling 传递进度。
>
> **异常防护**：后台线程中的 `ExperimentRunner::run()` 的整体调用包裹在 `try-catch` 中；
> 捕获 `std::exception` 与未知异常，通过 `LogPanel::log()` 记录到 UI 日志面板，
> 确保后台异常不会导致整个程序闪退。UI 主循环中的异常同理处理。

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
| M2 | FFT 估计器 + core/fft + rng + signal + window + metrics | 全部完整 |
| M3 | MUSIC/ESPRIT 骨架 | 骨架 |
| M4 | （已合并至 M2） | — |
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
| `core/fft.{h,cpp}` | 全部（公共 FFT 工具：computeDft / findPeaksFromDft；后者委托 PeakFinder） |
| `core/peak_finder.{h,tpp}` | 全部（通用一维寻峰模板：findPeaks + 中值滤波 / prominence / FWHM） |
| `core/rng.{h,cpp}` | 全部（四分布抽样：normal / uniform / laplace / impulse） |
| `signal/signal_generator.{h,cpp}` | 全部（正弦生成 + 干扰叠加 + 噪声注入） |
| `metrics/*.{h,cpp}` | 全部（PercentageError / RMSE / ComputeTime，含各自 `format()` / `showDistribution()` 实现） |
| `experiment/statistics.{h,cpp}` | 全部 |
| `experiment/experiment_runner.{h,cpp}` | 全部（按 §7.4 规约） |
| `src/ui/**` | 全部（含 DPI、字体、主循环、所有面板与控件） |
| `src/app/main.cpp` | 全部（GUI 启动） |
| `CMakeLists.txt` 改动 | 全部（option、源文件注册） |

### 12.3 用户自行完成的部分

| 部分 | 责任方 |
|---|---|
| `window/window.{h,cpp}` | 用户（已完成） |
| `estimator/fft_peak.cpp` 峰值估计（可调用 core/fft） | 用户（可已完成） |
| `estimator/fft_interpolate.cpp` 插值估计（按 windowKind 分支） | 用户（已完成） |
| `estimator/music.cpp` / `esprit.cpp` | 用户 |

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
| OQ-8 | `PeakFinder` 位置与 API 形态 | 放置于 `include/ispp/core/peak_finder.h`（Core 层"工具"职责）；类模板 `template <std::floating_point T>`；**所有方法均为 `static`**（修正参考签名中 `static ... const` 的 C++ 合规问题）；无实例状态，通过 `PeakFinder<double>::findPeaks(...)` 调用 |
| OQ-9 | `findPeaksFromDft` 签名稳定性 | **公开签名保持不变**（`threshold_factor` + `max_peak_count`）；实现内部委托 `PeakFinder<double>::findPeaks`，旧参数映射为 (margin, min_prominence) 与后置截断；调用方零改动 |
| OQ-10 | `PeakFinder` 模板文件组织 | 实现 `.tpp` 由 `.h` 末尾 `#include` 引入（header-only 模板）；`.tpp` 不参与 `target_sources`；CMake 无需改动（`include/` 已在 include path） |
| OQ-11 | 评价指标启用方式 | **全部启用**（移除配置面板的多选勾选 `MetricsMask` 与 Metrics 区域；`ExperimentRunner` 始终注册全部三个指标） |
| OQ-12 | RMSE 正确性与面板展示 | `RmseMetric::evaluate()` 单次返回 `(Δf)²`（保持不变）；MC 聚合后 `mean = MSE`，`sqrt(mean)` = RMSE；`name()` 改为 `"RMSE"`（原 `"MSE"`）；结果面板中 `showDistribution() = false`，仅显示单一 RMSE 值，不展示统计分布 |
| OQ-13 | 指标显示格式 | `IMetric` 新增 `format(double)`；各指标按自身规则格式化：百分比误差保留三位小数 + `%`；RMSE 对 MSE 开根号后 `%.6e`；计算时间 SI 单位 + 3 位有效数字 |
| OQ-14 | 双重 shutdown 崩溃 | `UiManager::run()` 末尾调用 `shutdown()` 后析构函数再次调用 → `ImGui_ImplOpenGL3_Shutdown` 断言失败。修复：仅在析构函数中调用 `shutdown()`（RAII），`run()` 末尾不再调用 |
| OQ-15 | 频谱轴自动重拟合 | `SetupAxisLimits` 默认 `ImPlotCond_Once` 仅首帧生效。修复：`SpectrumPanel` 跟踪输入信号堆指针检测数据变更，变更帧使用 `ImPlotCond_Always` 强制重设，其余帧 `ImPlotCond_Once` 保留用户缩放 |
| OQ-16 | 干扰频率参考线 | `RunResult` 新增 `LastInterferenceDeltaHz`（= `deltaBins × binHz`）；频谱面板在非零时绘制 `TRUE_FREQ + deltaHz` 的竖线 |
| OQ-17 | 信号幅度参数移除 | `SignalSpec` 不再包含 `Amplitude` 字段；信号生成固定幅度 1.0。干扰 `InterferenceSpec::Amplitude` 保留，但其语义为相对于信号基准（1.0）的线性比值。噪声功率计算相应简化（信号 RMS = 1/√2）。UI 配置面板移除信号幅度控件 |

---

## 15. 未来扩展点（非本版实现）

- **维度扫描**：把 5 个维度任意子集改造为 sweep 列表；`ExperimentRunner` 内层增加 sweep 嵌套循环，外层仍为蒙特卡洛。`ExperimentConfig` 应预留扩展形态。
- **结果导出**：CSV/JSON。
- **多算法对比**：同一配置下并列运行多种算法，结果面板表格按算法分行。
- **国际化（i18n）**：已通过 GNU gettext 实现（双文本域 `ui`/`con`，Win32 locale 自动检测，zh_CN 翻译）；不在本文档范围内，属于用户自主扩展。
