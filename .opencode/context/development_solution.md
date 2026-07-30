# 信号频率估计仿真实验框架 — 开发计划

| 项目 | 内容 |
|---|---|
| 文档版本 | v1.9 |
| 制定日期 | 2026-07-19 |
| 最近修订 | 2026-07-30（metric 身份键重构：name() 返回英语 msgid，worker 线程零 gettext OQ-32） |
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
│       │   ├── metric.h            ← IMetric 接口（含聚合指标扩展：isAggregate / finalize）
│       │   ├── percentage_error.h
│       │   ├── mse.h               ← 原 rmse.h；MSE = 1/M·Σ(Δf)²（不再开根号）
│       │   ├── compute_time.h
│       │   └── relative_efficiency.h  ← 新增：η = CRB / SampleVariance（聚合指标）
│       └── experiment/
│           ├── experiment_config.h ← 单一配置 + MonteCarloConfig
│           ├── experiment_runner.h ← 蒙特卡洛循环（完整实现）
│           ├── scan_test_runner.h  ← 批量扫描测试（M4）
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
│   │   ├── mse.cpp                 ← 原 rmse.cpp
│   │   ├── compute_time.cpp
│   │   └── relative_efficiency.cpp  ← 新增
│   ├── experiment/
│   │   ├── experiment_runner.cpp   ← 完整实现
│   │   ├── scan_test_runner.cpp    ← 批量扫描测试（M4）
│   │   └── statistics.cpp          ← 完整实现
│   └── ui/
│       ├── ui_manager.h/.cpp       ← GLFW/ImGui/ImPlot 初始化 + 主循环 + DPI
│       ├── panels/
│       │   ├── config_panel.h/.cpp
│       │   ├── spectrum_panel.h/.cpp
│       │   ├── results_panel.h/.cpp
│       │   ├── scan_results_panel.h/.cpp  ← 扫描测试图表（M4）
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

#include <complex>
#include <vector>

namespace ispp {

using RealArray = std::vector<double>;
using ComplexArray = std::vector<std::complex<double>>;

/// Denotes that amplitude of such frequency is unknown.
/// Never assume a specific value and should be handled properly.
const double AMP_UNKNOWN = -1;

/// Denotes that prominence of such frequency is unknown / meaningless.
/// Used by estimators that do not obtain peaks via PeakFinder (e.g. FFT
/// Interpolate produces a single interpolated frequency with no associated
/// prominence). See OQ-18.
const double PROMINENCE_UNKNOWN = -1;

// 单个估计出的频率点
struct FrequencyPeak {
    double FrequencyHz;
    double Amplitude;
    double Prominence; // 来自 PeakFinder 的拓扑突出度（OQ-18）
};

// 估计算法完整输出（含频率-幅度-突出度三元组列表 + 计时）。
// Peaks 由 IEstimator 填充，ComputeTimeSec 由 ExperimentRunner 外部注入。
struct EstimationResult {
    std::vector<FrequencyPeak> Peaks;
    double ComputeTimeSec;
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
///                      .Amplitude   = mags[idx],
///                      .Prominence  = p.Prominence }`；
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
/// MUSIC 频率估计算法（用户已实现 — beam-space MUSIC via Eigen SVD）。
class MusicEstimator final : public IEstimator {
public:
    explicit MusicEstimator(double threshold = 0.0);
    std::vector<FrequencyPeak>
    estimate(const RealArray& input, const EstimationContext& context) override;
    std::string_view name() const override;
private:
    double Threshold;
};

// esprit.h
/// ESPRIT 频率估计算法（用户已实现 — Hankel 矩阵 + 子空间旋转）。
/// @see Paper 10.1109/FOCS61266.2024.00137
/// @note 需要较高优化级别（如 -O3）才能在合理时间内完成计算
class EspritEstimator final : public IEstimator {
public:
    EspritEstimator() = default;
    std::vector<FrequencyPeak>
    estimate(const RealArray& input, const EstimationContext& context) override;
    std::string_view name() const override;
};
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
> **Prominence 语义**（OQ-18）：`FrequencyPeak::Prominence` 仅在经由
> `findPeaksFromDft`（内部委托 `PeakFinder`）产出时有物理意义——FFT Peak
> 直接获得拓扑突出度。FFT Interpolate 的最终频率来自二分搜索精修，无对应
> 峰值 → `Prominence = PROMINENCE_UNKNOWN`。MUSIC / ESPRIT 若对伪谱调用
> `PeakFinder<double>::findPeaks` 则获得伪谱域 Prominence（物理含义不同
> 于 DFT 域，但数值有效）；当前骨架阶段设为 `PROMINENCE_UNKNOWN`。
>
> **MUSIC / ESPRIT 的伪谱寻峰**：在伪谱（pseudospectrum）数组生成后，
> 应直接调用 `PeakFinder<double>::findPeaks(pseudospectrum, ...)`（§5.5），
> 而非把伪谱塞进 `findPeaksFromDft` —— `findPeaksFromDft` 内部会做
> `|dft[i]|` 复数取模，对已经是线性实数的伪谱是错误的额外步骤。
> 直接使用 `PeakFinder` 可获得完整的 (kernel_size / margin /
> min_prominence / min_width) 调参能力。
>
> **MUSIC / ESPRIT 性能优化注意事项**（用户实现总结）：
> 1. **大 N 优化（N > 256）**：MUSIC 与 ESPRIT 均使用滑动窗口 + Hankel
>    数据矩阵来降低计算复杂度。协方差矩阵通过 `SelfAdjointEigenSolver`
>    （Hermitian 特征值求解器）分解，而非通用 SVD——前者利用 Hermite 矩阵
>    结构，复杂度约为后者的一半。
> 2. **生产环境编译**：ESPRIT 的伪逆与特征值分解计算量大，建议启用
>    `-O3` 和 `-march=native`（或 `-march=x86-64-v3/v4`）以充分利用 SIMD
>    向量化。开发期 Debug 模式下执行时间可能慢 10× 以上。
>    参见 CMake 选项 `ISPP_ENABLE_NATIVE` / `ISPP_ENABLE_X86_64_V4` /
>    `ISPP_ENABLE_X86_64_V3`（§10.1）。

### 6.4 Metrics（**完整实现**）

`include/ispp/metrics/metric.h`

```cpp
#pragma once

#include "ispp/core/parameters.h" // NoiseInfo
#include "ispp/core/types.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ispp {

class IMetric {
public:
    IMetric() = default;
    IMetric(const IMetric &) = delete;
    IMetric &operator=(const IMetric &) = delete;
    IMetric(IMetric &&) = delete;
    IMetric &operator=(IMetric &&) = delete;
    virtual ~IMetric() = default;

    /// 单次评估：与真实频率比较，返回指标值。
    /// 多峰时选取误差最小的峰（OQ-6）。
    /// 聚合指标（isAggregate() == true）不需实现有效逻辑——
    /// Runner 不会调用此方法。
    virtual double evaluate(double trueFrequencyHz,
                            const EstimationResult& result) = 0;

    virtual std::string_view name() const = 0;

    /// 将统计值格式化为人类可读字符串（决策记录 OQ-13）。
    /// 各指标格式不同：百分比误差保留四位小数并加 `%` 后缀；
    /// MSE 直接以 `%.6e` 显示；计算时间用 SI 单位 + 3 位有效数字；
    /// 相对效率保留六位小数（NaN → "N/A"）。
    virtual std::string format(double value) const = 0;

    /// 是否在结果面板展示完整的统计分布（mean/std/min/max）。
    /// MSE 和 RelativeEfficiency 返回 false（仅显示单一值）。
    virtual bool showDistribution() const { return true; }

    // --- 聚合指标扩展（OQ-20）---------------------------------------------

    /// 是否为聚合指标（蒙特卡洛结束后一次性计算，而非每轮迭代评估）。
    /// 聚合指标使用 finalize() 而非 evaluate() + computeStats()。
    virtual bool isAggregate() const { return false; }

    /// 聚合指标的后处理计算（MC 循环结束后调用一次）。
    ///
    /// @param freqEstimates 每轮迭代估计出的频率（原始 f_i 值）。
    /// @param sampleRateHz 采样率。
    /// @param sampleCount 采样点数 (N)。
    /// @param noiseInfo 噪声分布 + SNR。
    /// @return 指标值；NaN 表示不适用（如 Uniform/Impulse 分布的 CRB）。
    ///         Runner 将结果存入 MetricStats::Mean；
    ///         结果面板经 format() 格式化后单行显示。
    virtual double finalize(const std::vector<double>& freqEstimates,
                           double sampleRateHz,
                           std::size_t sampleCount,
                           NoiseInfo noiseInfo) const;
};

} // namespace ispp
```

| 实现 | 类型 | 行为 | 返回值（单次 / 聚合） | 结果面板展示 | `format()` 规则 |
|---|---|---|---|---|---|
| `PercentageErrorMetric` | 逐轮 | 与 `result.Peaks` 中**误差最小的峰**比较（OQ-6），返回百分比 | `\|Δf\| / f_true × 100%` | `showDistribution() = true`：完整统计列 | 保留四位小数 + `%` 后缀，如 `0.1500%` / `12.3456%` |
| `MseMetric` | 逐轮 | 均方误差：单次返回 `(Δf)²`；MC 聚合后 `mean = MSE = 1/M·Σ(Δf)²`；选峰策略 = **max-Prominence**（OQ-21） | `(Δf)²` | `showDistribution() = false`：**仅显示单一 MSE 值**；不展示统计分布 | 直接以 `%.6e` 显示（如 `1.234e-06`），**不再开根号** |
| `ComputeTimeMetric` | 逐轮 | 直接返回 `result.ComputeTimeSec` | `ComputeTimeSec` | `showDistribution() = true`：完整统计列 | SI 单位 + 3 位有效数字，如 `375ns` / `5.52us` / `10.5ms` / `1.23s` |
| `RelativeEfficiencyMetric` | 聚合 | 相对效率：`η = CRB / SampleVariance`（OQ-20）；高斯/拉普拉斯有 CRB 解析式，均匀/脉冲不满足正则条件→返回 NaN。**⚠️ 当前已禁用**——模型假设不完全匹配 CRB 条件；代码保留但未注册到 Runner / UI（见 OQ-20 更新） | `finalize()` 返回 η 或 NaN | `showDistribution() = false`：单行显示（MSE 下方） | NaN → `"N/A"`；有效值保留六位小数（如 `0.987654`） |

#### 6.4.1 RelativeEfficiencyMetric 实现规约（OQ-20）

**CRB（Cramér–Rao 下界）解析式**——单频正弦信号（幅度 = 1.0）叠加加性噪声：

| 噪声分布 | CRB 表达式 | 正则条件 |
|---|---|---|
| Gaussian | `6·fs²·σ² / (π²·N·(N²−1))` | ✅ 满足 |
| Laplacian | `3·fs²·σ² / (π²·N·(N²−1))` | ✅ 满足 |
| Uniform | — | ❌ 不满足（密度函数不处处可导） |
| Impulse | — | ❌ 不满足（离散概率质量） |

其中 `σ`（噪声标准差）由 SNR 推导（信号幅度固定 1.0）：

```text
信号功率 = A²/2 = 0.5
噪声功率 = 0.5 / 10^(SNR_dB / 10)
σ = √(噪声功率)
```

**SampleVariance**（估计频率的样本方差）：

```text
f_avg = 1/M · Σ f_i
SampleVariance = 1/(M−1) · Σ (f_i − f_avg)²
```

> 注意：`computeStats()` 内部已对 `metric_samples` 计算 `std = √(sample_variance)`，
> 但那是针对 `(Δf)²` 的方差（MSE 指标的样本），不是频率估计本身的方差。
> RelativeEfficiency 需要的是**原始 f_i 的样本方差**，因此 Runner 单独收集
> `freqEstimates` 并传入 `finalize()`。

**`finalize()` 返回值**：

```text
若 noiseInfo.Distribution ∈ {UNIFORM, IMPULSE}:
    return NaN     // CRB 未定义 → 面板显示 "N/A"
若 M < 2:
    return NaN     // 无法计算样本方差
否则:
    η = CRB / SampleVariance
    return η       // 典型范围 [0, 1]；越接近 1 表示估计器越高效
```

**`format()` 规则**：

```cpp
std::string format(double value) const {
    if (std::isnan(value))
        return "N/A";
    return std::format("{:.6f}", value);
}
```

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
 2. 为每个非聚合 metric 准备 samples: vector<vector<double>>
    检查是否存在聚合 metric（hasAggregate = any_of(metrics, isAggregate)）
    若 hasAggregate：声明 vector<double> freqEstimates（收集原始 f_i）
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
       frequencyCount = config.maxFreqCount +
                        (config.env.interference.deltaBins != 0 ? 1 : 0);
       NoiseInfo noiseInfo{config.env.noise.distribution, config.env.noise.snrDb};
       EstimationContext ctx{/*sampleRateHz*/ sampleRate,
                             /*windowKind*/   config.env.window.kind,
                             /*frequencyCount*/ frequencyCount,
                             /*noiseInfo*/     noiseInfo};
       auto peaks = estimator_->estimate(input, ctx);
       ```

    g. 计时结束 → 组装 EstimationResult{std::move(peaks), computeSec}
    h. for each metric m:
         若 metric->isAggregate()：skip（聚合指标不参与逐轮评估）
         否则：samples[m].push_back(metric->evaluate(trueFreq, er))
    i. 若 hasAggregate 且 er.Peaks 非空：
         bestFreq = Prominence 最大的峰的频率（OQ-21 选峰策略）
         freqEstimates.push_back(bestFreq)
    j. 若 i == iterationCount - 1：缓存 LastInputSignal / LastSpectrum* / LastPeaks
       （频谱通过 computeDft(input) 计算，复用 core/fft）
    k. onProgress((i + 1) / iterationCount)
 5. for each metric m:
      若 metric->isAggregate():
        value = metric->finalize(freqEstimates, sampleRate, N, noiseInfo)
        result.Metrics.push_back({m, MetricStats{.Mean = value}})
      否则:
        result.Metrics.push_back({m, computeStats(samples[m])})
 6. result.TotalRuntimeSec = 全程计时
 7. return result;
```

**关键不变量**：
- Runner 仅依赖 `IEstimator` 和 `IMetric` 接口；用户实现算法后**无需修改 Runner**。
- `iterationCount == 1` 时 `MetricStats` 四字段均等于该次单值。
- RNG 种子策略保证相同 `baseSeed` 下结果可复现。
- 聚合指标（`isAggregate() == true`）的 `MetricStats` 仅使用 `Mean` 字段；
  `Std` / `Min` / `Max` 初始化为零但因 `showDistribution() == false` 不会被显示。
- `freqEstimates` 仅在存在聚合 metric 时收集，避免无谓开销。

---

## 7.5 批量扫描测试 — 数据结构（M4，**完整实现**）

> 批量扫描测试在控制其余自变量不变的前提下，改变一个或多个参数，
> 对每组参数组合执行完整的蒙特卡洛实验，收集指定 metric 的统计结果，
> 最终绘制图表。测试规格（默认参数、扫描范围、metric 选择、图表样式）
> **硬编码**在 `ScanTestRunner::buildDefaultTests()` 中，不在 UI 中暴露。
>
> 支持三种维度角色（OQ-26）：
> - **X 轴变量**（必选）：图表横轴，逐点扫描
> - **系列变量**（可选）：同一图表内的多条数据线/柱组，用颜色或线型区分
> - **分图变量**（可选）：每个取值生成一张独立图表

### 7.5.1 全局默认参数

```text
SampleRateHz     = 1000.0
SampleCount      = 256
FrequencyHz      = 200.0
PhaseRad         = 0.0
NoiseDistribution = GAUSSIAN
SnrDb            = 10.0
WindowKind       = RECTANGULAR
DeltaBins        = 0.0  (无干扰)
Amplitude        = 0.5
MaxFreqCount     = 1
IterationCount   = 100
BaseSeed         = 7792565
```

### 7.5.2 可扫描参数 + 算法注册表

```cpp
// include/ispp/experiment/scan_test_runner.h

enum class ScanParam : std::uint8_t {
    // 连续型
    SnrDb, FrequencyHz, PhaseRad, InterferenceDeltaBins, InterferenceAmplitude,
    // 离散型（数值）
    SampleCount, MaxFreqCount,
    // 离散型（类别）
    WindowKind, NoiseDistribution,
    // 特殊：不在 ExperimentConfig 中，需单独解析为 IEstimator
    Algorithm,
};

bool isScanParamDiscrete(ScanParam param);
void applyScanParam(ExperimentConfig &config, ScanParam param, double value);

/// 算法注册表——四种估计器的名称 + 构造器。
/// 扫描维度涉及 Algorithm 时，按索引取值。
struct AlgorithmEntry {
    std::string Name;
    std::shared_ptr<IEstimator> Estimator;
};
static const std::vector<AlgorithmEntry> ALL_ALGORITHMS = {
    {"FFT Peak",       std::make_shared<FftPeakEstimator>(0.0)},
    {"FFT Interpolate", std::make_shared<FftInterpolateEstimator>(0.0)},
    {"MUSIC",          std::make_shared<MusicEstimator>(0.0)},
    {"ESPRIT",         std::make_shared<EspritEstimator>()},
};
```

### 7.5.3 图表样式枚举

```cpp
enum class ChartStyle {
    /// 折线 + 误差带：粗线=均值，深色带=±std，浅色带=min/max。
    /// 用于单系列、连续或离散 X 轴，且 metric 有完整分布统计时。
    LineWithErrorBands,

    /// 分组柱状图 + 误差须：X 轴类别分组，组内多色柱代表系列变量。
    /// 柱高=均值，须线=min~max。用于多系列离散参数。
    GroupedBarsWithError,

    /// 多折线（均值）：X 轴连续，不同系列用不同线型（实线/虚线/点线/点划线）。
    /// 不显示误差带。用于仅需均值对比的多系列场景。
    MultiLine,
};
```

### 7.5.4 测试规格与结果数据结构

```cpp
/// 一条扫描维度的取值序列。
struct ScanDimension {
    ScanParam Param;
    std::vector<double> Values;
    std::vector<std::string> Labels; // 离散参数的类别标签（平行于 Values）
};

/// 完整测试定义。
struct ScanTestDef {
    std::string Name;

    ScanDimension XDim;                    // 必选：X 轴变量
    std::optional<ScanDimension> SeriesDim; // 可选：同图系列变量
    std::optional<ScanDimension> ChartDim;  // 可选：分图变量

    std::vector<std::string> MetricNames;  // 每个 metric → 一组分图
    ChartStyle Style;

    /// 配置覆盖（区别于全局默认的常量值）。
    std::vector<std::pair<ScanParam, double>> Overrides;

    /// 固定估计器（当 Algorithm 不是 ChartDim/SeriesDim 时使用）。
    std::shared_ptr<IEstimator> FixedEstimator;

    /// 若为 true，跳过标准 metric 管线，改为逐峰提取百分比误差。
    /// MC=1，从 LastPeaks 计算每个峰的 |freq−true|/true×100%，按误差
    /// 升序排列，每个排位画一条 MULTI_LINE 折线（OQ-28）。
    bool PerPeak = false;
};

/// 单个系列的统计数据（平行于 XDim.Values）。
struct SeriesResult {
    std::string Name;               // 图例标签
    std::vector<double> Means;
    std::vector<double> Stds;       // 空 = metric 无分布统计
    std::vector<double> Mins;
    std::vector<double> Maxs;
};

/// 单张图表的完整数据。
struct ChartResult {
    std::string Title;
    std::string XLabel, YLabel;
    ChartStyle Style;
    std::vector<double> XValues;
    std::vector<std::string> XLabels; // 离散时使用
    bool IsDiscrete;
    std::vector<SeriesResult> Series;
};

/// 单次测试的输出（包含若干张图表）。
struct ScanTestOutput {
    std::string Name;
    std::vector<ChartResult> Charts;
};
```

> **Metric 提取**：每次实验后遍历 `RunResult::Metrics`，
> 按 `MetricName` 与 `IMetric::name()` 匹配，取 `Stats` 全部四个字段。
> `showDistribution() == false` 的 metric 仅 `Mean` 有效。

---

## 7.6 `ScanTestRunner`（**完整实现**）

### 7.6.1 接口

```cpp
class ScanTestRunner {
public:
    using ProgressCallback =
        std::function<void(float, const std::string &)>;

    ScanTestRunner(); // 自动调用 buildDefaultTests()

    std::vector<ScanTestOutput>
    run(const ProgressCallback &onProgress = nullptr);

    void cancel();
    bool isCancelled() const;

private:
    std::vector<ScanTestDef> Tests;
    std::atomic<bool> Cancelled{false};
    static std::vector<ScanTestDef> buildDefaultTests();
};
```

### 7.6.2 多维枚举流水线

```text
run():
 1. totalPoints = Σ (xValues × seriesValues × chartValues × metrics)
                   for all tests
    completed = 0; outputs = []
 2. for each test in Tests:
    a. log("Starting: " + test.Name)
    b. chartVals = test.ChartDim ? ChartDim.Values : [single]
       seriesVals = test.SeriesDim ? SeriesDim.Values : [single]
    c. for each cv in chartVals:         // 分图维度
       for each metricName in test.MetricNames:  // 每个 metric 一组分图
          chart = {title(test.Name, cv, metricName), ...}
          for each sv in seriesVals:     // 系列维度
             series = {label(sv), [], [], [], []}
             for each xv in test.XDim.Values:  // X 轴维度
                i.  若 isCancelled() → break all
                ii. config = globalDefaults
                    applyOverrides(config, test.Overrides)
                    applyScanParam(config, XDim.Param, xv)
                    if SeriesDim: applyScanParam(config, SeriesDim.Param, sv)
                    if ChartDim: applyScanParam(config, ChartDim.Param, cv)
                iii.estimator = resolveEstimator(test, cv, sv)
                    metrics = buildMetrics()  // 注册全部 metric
                    runner = ExperimentRunner(config, estimator, metrics)
                    result = runner.run()
                iv.  找到 metricName 对应的 MetricResult:
                    series.Means.push_back(stats.Mean)
                    series.Stds.push_back(stats.Std)   // 若 showDistribution()
                    series.Mins.push_back(stats.Min)
                    series.Maxs.push_back(stats.Max)
                v.   completed++; onProgress(completed/total, logMsg)
             chart.Series.push_back(series)
          output.Charts.push_back(chart)
    d. outputs.push_back(output)
       log("Completed: " + test.Name)
 3. return outputs
```

**`resolveEstimator(test, cv, sv)`**：若 `ChartDim.Param == Algorithm` 则
按 `cv` 索引 `ALL_ALGORITHMS`；若 `SeriesDim.Param == Algorithm` 则按 `sv`；
否则返回 `test.FixedEstimator`。

---

## 7.7 具体测试定义（硬编码于 `buildDefaultTests()`）

> 全局默认参数见 §7.5.1。下表反映 **v1.8 重构后** 的规格（OQ-29）。
> 重构要点：Tests 1/2/4 由"分图=算法"改为"系列=算法 + MULTI_LINE"（4 算法
> 同图 4 线，仅均值，无误差带）；Test 5 增补 MUSIC/ESPRIT 两张分图；
> Test 6 的 SNR 由固定覆盖改为分图维度（−3 dB / 10 dB）。
> 后果：`LineWithErrorBands`（Style A）已无任何测试引用（见 §8.8）。

| # | 名称 | X 轴 | 系列变量 | 分图变量 | 覆盖 | Metric | 样式 | 图表数 |
|---|---|---|---|---|---|---|---|---|
| 1 | SampleCount scan | SampleCount: 32, 64, 128, 256, 512, 1024 | Algorithm(4) | — | — | PE, CT | MultiLine | 2 |
| 2 | Frequency scan | Freq: 1500~1530 step 3 (11pts) | Algorithm(4) | — | SR=7680 | PE | MultiLine | 1 |
| 3 | NoiseDist × Algorithm | NoiseDist(4) | Algorithm(4) | — | SNR=−8 | PE | GroupedBarsWithError | 1 |
| 4 | SNR scan | SNR: −30~20 step 2.5 (21pts) | Algorithm(4) | — | — | PE | MultiLine | 1 |
| 5 | SNR × SampleCount | SNR: −30~20 step 2.5 (21pts) | SampleCount: 64,128,256,512 | Algorithm: Interpolate,MUSIC,ESPRIT(3) | — | PE | MultiLine | 3 |
| 6 | Window × Algorithm | Algorithm: Interpolate,MUSIC,ESPRIT(3) | WindowKind(4) | SNR: −3,10 dB(2) | — | PE | GroupedBarsWithError | 2 |
| 7 | Interference scan | DeltaBins: 0~4 step 0.2 (21pts) | — | Algorithm(4) | — | PerPeak | MultiLine | 4 |

**总计 14 张图表。**（v1.7 为 23 张；Tests 1/2/4 由每算法一张折线带误差图合并为单张多线图，Test 5 由 1 张增为 3 张，Test 6 由 1 张增为 2 张。）

**缩写**：PE = PercentageError, CT = ComputeTime, SR = SampleRateHz, PerPeak = 逐峰百分比误差模式（MC=1，按距真频排序，每排位一条折线）

### 各测试详细规格

**Test 1 — SampleCount scan（v1.8：algo 作系列，MULTI_LINE）**
- 默认配置 + 无覆盖；4 种算法作为 4 条系列线同图对比（仅均值）
- 每个 metric 一张图（PE、CT）→ 共 2 张 MULTI_LINE 图
- X 轴 6 个点（32/64/128/256/512/1024）

**Test 2 — Frequency scan（v1.8：algo 作系列，MULTI_LINE）**
- 覆盖 `SampleRateHz = 7680`（确保 Nyquist 覆盖 1530 Hz）
- 4 种算法作 4 条系列线；X 轴 11 点（1500/1503/.../1530 Hz），仅 PE → 1 张图

**Test 3 — NoiseDist × Algorithm**（未改动）
- 覆盖 `SNR = −8 dB`；X 轴 = 4 种噪声分布；系列 = 4 种算法（同图 4 色）
- 柱状分组：每组 4 根柱（算法），柱高 = PE 均值，误差须 = min~max

**Test 4 — SNR scan（v1.8：algo 作系列，MULTI_LINE；移除 GenerateOverview）**
- 默认配置；4 种算法作 4 条系列线；X 轴 21 点（−30~...~20 dB），仅 PE → 1 张图
- 不再生成总览图（原 v1.7 的 overview 等价于现单张多线图，已冗余）

**Test 5 — SNR × SampleCount（v1.8：按算法分图，新增 MUSIC/ESPRIT）**
- 分图 = Algorithm {FFT Interpolate, MUSIC, ESPRIT}（排除 FFT Peak）→ 3 张图
- 每图系列 = 4 种采样数（N=64/128/256/512）；X 轴 = SNR(21 点)，仅 PE
- `FixedEstimator` 不再使用（Algorithm 升为 ChartDim）

**Test 6 — Window × Algorithm（v1.8：按 SNR 分图，新增 10 dB）**
- 分图 = SNR {−3 dB, 10 dB} → 2 张图；X 轴 = 3 种算法（Interpolate/MUSIC/ESPRIT）
- 系列 = 4 种窗函数；柱状分组，柱高 = PE 均值，误差须 = min~max
- SNR 不再是固定覆盖，改为 ChartDim

**Test 7 — Interference scan（PerPeak 模式, OQ-28；未改动）**
- 默认配置；X 轴 21 点（0~0.2~...~4.0 bins）；4 种算法各一张图
- `PerPeak = true`：MC=1（确定性单次运行），跳过标准 metric 管线
- 从 `RunResult::LastPeaks` 提取所有峰，计算各自的 `|freq − true| / true × 100%`
- 每个 X 点的峰按误差升序排列（rank 0 = 最接近真频）
- 每个排位画一条折线（图例 `Peak %d`，UI 线程本地化为"峰 N"），排位不足的 X 点用 NaN 断线
- 样式：MULTI_LINE（仅均值，无误差带）

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
│   ├── scan_results_panel.h/.cpp ← 扫描测试 ImPlot 图表（M4）
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
- 评价指标：**全部始终启用**（不再提供勾选；移除 MetricsMask / Metrics 多选区域；`ExperimentRunner` 始终注册三个指标：PercentageError、MSE、ComputeTime。RelativeEfficiencyMetric 暂未注册——见 OQ-20）
- **"运行" 按钮**：触发后台 `ExperimentRunner::run()`
- **"运行扫描测试" 按钮**：一键启动所有硬编码的扫描测试（M4, OQ-22）；
  内部调用 `ScanTestRunner::run()`，在后台线程顺序执行。
  运行时复用进度条（显示全局扫描进度），并在日志面板输出每个测试的
  开始 / 结束消息。

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

- 遍历 `Metrics`，按各 metric 的 `showDistribution()` 分两种展示：

  | `showDistribution()` | 展示方式 |
  |---|---|
  | `true`（PercentageError、ComputeTime） | 表格行：`Metric \| Mean \| Std \| Min \| Max`；每个统计值经 `metric.format()` 格式化后显示 |
  | `false`（MSE、RelativeEfficiency） | 单行：`"<name>: <formatted_value>"`；MSE 直接显示 `1/M·Σ(Δf)²`（`%.6e`）；RelativeEfficiency 显示 `η`（六位小数如 `0.987654`）或 `"N/A"`（Uniform / Impulse 分布） |

- `iterationCount == 1` 时仅显示单值列（其他列隐藏或显示同值）
- 末次 `computeTimeSec` 单独标注

### 8.6 日志面板

- 应用内 `std::string` 消息缓冲（环形缓冲，`MAX_LOG = 200`）
- **线程安全**：`log()` 与 `render()` 均通过 `std::mutex` 互斥
- **渲染安全（OQ-27）**：`render()` 在锁内将消息复制到 `RenderCopy` 向量，再从副本渲染。`RenderCopy` 在下一帧 `render()` 开头清空（此时上一帧的 `ImGui::Render()` 已消费了指针）。`NextIdx` 无限递增，回绕时用 `NextIdx % MAX_LOG` 分两段遍历，避免越界。
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

    configPanel.render(sharedConfig, runRequest, scanRequest)
    spectrumPanel.render(lastRunResult)
    resultsPanel.render(lastRunResult)
    scanResultsPanel.render(scanResults)
    logPanel.render()

    若 runRequest 为真 且 后台无任务:
        启动 std::thread 执行 ExperimentRunner::run(progressCb)
        UI 端用 progressCb 更新进度条
    若后台完成 → 拷贝 RunResult 到 lastRunResult

    若 scanRequest 为真 且 后台无任务:
        启动 std::thread 执行 ScanTestRunner::run(progressCb)
        进度条显示全局扫描进度（completedPoints / totalPoints）
        日志输出每个测试的开始/结束消息
    若后台扫描完成 → 拷贝 vector<ScanTestResult> 到 scanResults

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

### 8.8 扫描测试结果面板（ImPlot）— M4

**布局**：与频谱面板（§8.4）一致——每张图表各自包裹在
`ImGui::BeginChild(..., ImGuiChildFlags_ResizeY)` 容器中（ID = 英文组合
`chart.Title`，不受 locale 影响），从上到下依次排列，用户可垂直拖拽调整各图高度。

**空状态**：无扫描结果时显示灰色提示文本（已 i18n：`_UI("No scan results — run scan tests first.")`）。

**完整 i18n（OQ-30）**：所有可见文本均在 **UI 线程** 经 `_UI()` 本地化——
worker 线程（`ScanTestRunner::run()`）只存英语 msgid 字面量，绝不调用 `_UI()`。
- `chart.Title` 为英文组合串（"TestName — YLabel [ChartDimLabel]"），仅作
  ImGui/ImPlot 稳定唯一 ID；显示标题由 `localizedTitle()` 从原子分量
  `TestName`/`YLabel`/`ChartDimLabel`/`IsOverview` 重新组合（这些字段为
  `ChartResult` 在 v1.8 新增）。
- 轴标签 / 图例 / 刻度类别经 `_UI()` 即时翻译；刻度标签数组先物化进
  `std::vector<std::string>`（`makeLocalizedLabelPtrs`）再取 `.c_str()`，规避
  dgettext 静态缓冲区被下一次调用覆盖的陷阱。
- 逐峰图例 `Peak %d` 为运行期拼接串，gettext 无法整体匹配；改为在
  `SeriesResult` 上携带 `PeakRank`，UI 线程用 `localizedSeriesLabel()` 以
  翻译后的 `"Peak %d"` 格式（`snprintf`）生成（如 zh_CN → "峰 N"）。

**三种图表样式渲染逻辑**（OQ-24）：

#### Style A: LineWithErrorBands（折线 + 误差带）—— ⚠️ v1.8 后无测试引用

> v1.8 重构后 Tests 1/2/4 全部改为 MULTI_LINE，**没有任何测试再选用此样式**。
> 渲染代码（`renderLineWithErrorBands`）保留以备未来使用，但当前为死路径。

```text
// 三层叠加，从后往前画（浅色在后）
PlotShaded("min~max", xs, mins, maxs, n)       // 最浅色填充
PlotShaded("±std",    xs, meanMinusStd,
                        meanPlusStd, n)         // 中等色填充
PlotLine("mean",      xs, means, n, thickness=2) // 粗线
```

#### Style B: GroupedBarsWithError（分组柱状 + 误差须）

用于多系列离散参数（Tests 3/6）。X 轴分组，组内 N 根柱（N = 系列数）。

```text
// 每组宽度 = 1.0，柱宽 = 1.0 / (N+1)
for each series idx:
    barXs = groupCenter + (idx - (N-1)/2) * barWidth  // 柱中心偏移
    PlotBars(seriesName, barXs, means, n, barWidth)
    // 误差须：每根柱中心绘制 min~max 的竖线 + 短横帽
    for each point:
        PlotErrorBars("", barX, mean, min, max)
// X 轴刻度标签 = 组类别标签（如 "Gaussian", "Uniform", ...）
SetupAxisTicks(X1, groupCenters, groupLabels)
```

> v1.8：移除原先硬编码的 `SetupAxisLimits(X1, center0−0.6, centerN−1+0.6)`，
> X 轴范围交由自动拟合 + FitPadding 统一处理（见下）。

#### Style C: MultiLine（多折线，均值）

用于多系列场景，仅对比均值。**v1.8 后覆盖 Tests 1/2/4/5/7**（不同系列用不同
颜色 + 标记类型区分；算法系列=4 条线，采样数系列=4 条线，逐峰排位=N 条线）。

```text
for each series idx:
    ImPlot::SetNextLineStyle(color, thickness, styleIdx) // 颜色 + 标记
    PlotLine(seriesName, xs, means, n)
// 图例自动显示系列名称（经 localizedSeriesLabel 本地化）
```

**图表留白与尺寸（OQ-31，v1.8 新增）**：
- **5% 双侧留白**：`render()` 在绘制所有图表前后 `ImPlot::PushStyleVar(
  ImPlotStyleVar_FitPadding, ImVec2(0.1f, 0.1f))` / `PopStyleVar()`。
  ImPlot `ApplyFit` 对每侧增加 `(range/2)×padding`，故 `0.1` → 每侧 5%
  数据范围（X/Y 均适用）。作用域仅扫描面板，不影响频谱面板。
- **图表宽度收窄**：`plotSize()` 返回 `availableWidth − fontSize×2.5`，为最
  右侧 X 轴刻度标签留出空间，避免被子容器右边框遮挡（按字号缩放以适配 DPI）。
- **轴适配策略**：除上述 FitPadding 外，沿用默认自动拟合（首帧拟合 +
  FitPadding，之后保留用户缩放），与频谱面板（§8.4 OQ-15）一致。

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
| M3 | MUSIC/ESPRIT | MUSIC + ESPRIT 均由用户实现（beam-space MUSIC + Hankel ESPRIT，via Eigen） |
| M4 | （已合并至 M2）Metrics + Statistics；**批量扫描测试（扫描架构 + ImPlot 图表）** | Metrics/Statistics 已完成；扫描测试架构已设计（§7.5~§7.6, §8.8），待实现 |
| M5 | UI 完整 + main.cpp | **完整实现** |
| M6 | 端到端冒烟（用户实现任一算法后即可跑） | ✅ **已完成** — 用户已验证全部 4 个算法可运行 |

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
| `metrics/*.{h,cpp}` | 全部（PercentageError / MSE / ComputeTime / RelativeEfficiency，含各自 `format()` / `showDistribution()` / `isAggregate()` / `finalize()` 实现） |
| `experiment/statistics.{h,cpp}` | 全部 |
| `experiment/experiment_runner.{h,cpp}` | 全部（按 §7.4 规约） |
| `experiment/scan_test_runner.{h,cpp}` | 全部（按 §7.5~§7.6 规约：ScanParam / ScanTestSpec / ScanTestRunner 流水线） |
| `src/ui/**` | 全部（含 DPI、字体、主循环、所有面板与控件，含扫描测试结果面板 §8.8） |
| `src/app/main.cpp` | 全部（GUI 启动） |
| `CMakeLists.txt` 改动 | 全部（option、源文件注册） |

### 12.3 用户自行完成的部分

| 部分 | 责任方 |
|---|---|
| `window/window.{h,cpp}` | 用户（已完成） |
| `estimator/fft_peak.cpp` 峰值估计（可调用 core/fft） | 用户（可已完成） |
| `estimator/fft_interpolate.cpp` 插值估计（按 windowKind 分支） | 用户（已完成） |
| `estimator/music.cpp` | 用户（**已完成** — beam-space MUSIC via Eigen SVD + pseudospectrum peak search） |
| `estimator/esprit.cpp` | 用户（**已完成** — Hankel 矩阵 + 子空间旋转 via Eigen；@see 10.1109/FOCS61266.2024.00137） |

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
| OQ-18 | FrequencyPeak.Prominence | `FrequencyPeak` 新增 `Prominence` 字段 + `PROMINENCE_UNKNOWN` 哨兵常量。FFT Peak 经由 `findPeaksFromDft`（内部委托 `PeakFinder`）获得有意义的拓扑突出度；FFT Interpolate 的精修频率无对应峰 → `PROMINENCE_UNKNOWN`；MUSIC / ESPRIT 若对伪谱调用 `PeakFinder` 则获得伪谱域 Prominence（当前骨架阶段设为 `PROMINENCE_UNKNOWN`） |
| OQ-19 | MSE 替代 RMSE | 撤销 OQ-12 的 RMSE 决策。`RmseMetric` 重命名为 `MseMetric`（文件 `rmse.{h,cpp}` → `mse.{h,cpp}`）；`evaluate()` 仍返回 `(Δf)²`；`format()` **不再开根号**——MC 均值 `= MSE = 1/M·Σ(Δf)²` 直接显示；`name()` 返回 `"MSE"` |
| OQ-20 | 相对效率聚合指标 | 新增 `RelativeEfficiencyMetric`（`isAggregate() = true`）：`η = CRB / SampleVariance`。CRB 对高斯分布 = `6fs²σ²/(π²N(N²-1))`、拉普拉斯分布 = `3fs²σ²/(π²N(N²-1))`（σ = 噪声标准差，由 SNR + 信号幅度 1.0 推导）；均匀/脉冲分布不满足正则条件 → NaN → 面板显示 "N/A"。SampleVariance = `1/(M-1)·Σ(f_i-f_avg)²`。`IMetric` 扩展 `isAggregate()` / `finalize()`；Runner 收集原始 `freqEstimates` 供聚合计算。**⚠️ 当前已禁用**——当前仿真模型假设（固定单频 + 可选干扰）与 CRB 正则化条件不完全匹配，实际使用中 η 无可靠意义。代码保留（仍参与编译以防 API 漂移）但未注册到 Runner / UI。重新启用需恢复 `experiment_runner.cpp` 聚合逻辑 + `config_panel.cpp` 注册 |
| OQ-21 | MSE / RelativeEfficiency 选峰策略 | 撤销 OQ-6 中"选取误差最小峰"的决策（仅对 MSE 和 RelativeEfficiency 生效）。改为选取 **Prominence 最大的峰**——因为实际频率估计中无法预知真实频率，只能依赖峰值本身的显著度来判定主峰。PercentageError 保留 OQ-6 的 min-error 策略（衡量估计器能达到的最优精度）。FFT Peak 经由 PeakFinder 提供有意义的 Prominence；FFT Interpolate 返回单峰时无歧义 |
| OQ-22 | 扫描测试规格来源 | **硬编码**在 `ScanTestRunner::buildDefaultSpecs()` 中（默认参数、扫描范围、metric 选择），不在 UI 中暴露。用户给出具体数值后填充。每个 `ScanTestSpec` 携带独立的 `DefaultConfig` / `Estimator` / `Metrics` |
| OQ-23 | 扫描测试 vs 主实验的关系 | `ScanTestRunner` 内部为每个扫描点创建独立 `ExperimentRunner` 实例（共享 `shared_ptr<IEstimator>` / `shared_ptr<IMetric>`），复用现有蒙特卡洛逻辑。扫描测试与主实验共享同一后台线程槽（`UiManager::Worker`），互斥运行 |
| OQ-24 | 扫描图表类型选择 | 三种 `ChartStyle`：**LineWithErrorBands**（折线+误差带：均值粗线 / ±std 深色带 / min~max 浅色带；Tests 1/2/4/7）；**GroupedBarsWithError**（分组柱状+误差须：X 轴类别分组，组内多色柱，须线=min~max；Tests 3/6）；**MultiLine**（多折线均值：不同线型区分系列；Test 5）。样式由 `ScanTestDef::Style` 硬编码指定 |
| OQ-25 | 扫描 metric 提取方式 | 按 **名称匹配**（`IMetric::name() == spec.MetricName`），从 `RunResult::Metrics` 中查找对应 `MetricStats`，提取全部四个字段（Mean/Std/Min/Max）。`showDistribution() == false` 的 metric 仅 Mean 有效 |
| OQ-26 | 扫描维度角色（X 轴 / 系列 / 分图） | 每条 `ScanDimension` 有三种角色：**XDim**（必选，图表横轴）；**SeriesDim**（可选，同图多色/多线型系列）；**ChartDim**（可选，每个取值生成独立图表）。Algorithm 作为特殊维度，由 `ALL_ALGORITHMS` 注册表解析为 `IEstimator`。若 Algorithm 不是 ChartDim/SeriesDim，则使用 `FixedEstimator` |
| OQ-27 | LogPanel 环形缓冲区越界 | `NextIdx` 无限递增（总写入计数），`render()` 以 `NextIdx` 为上界遍历 `Messages`。缓冲区回绕后（`NextIdx ≥ MAX_LOG`）循环越过 `Messages.size()` → 主线程 SIGSEGV。修复：回绕时用 `NextIdx % MAX_LOG` 分两段渲染；同时在锁内复制到 `RenderCopy` 向量再渲染，防止工作线程覆写导致悬垂 `c_str()` 指针 |
| OQ-28 | Test 7 逐峰百分比误差 | `PerPeak = true` 跳过标准 metric 管线。MC=1 确定性运行，从 `LastPeaks` 提取所有峰并计算各自百分比误差，按误差升序排列。每个排位（Peak 1 = 最接近真频）画一条 MULTI_LINE 折线；排位不足处用 NaN 断线。适用于可视化干扰导致的频率分裂 |
| OQ-29 | 扫描测试规格重构（v1.8） | Tests 1/2/4：Algorithm 由 ChartDim 改为 SeriesDim（4 算法同图 4 线）+ 样式改 MULTI_LINE（仅均值，弃用误差带）；Test 1 保留双 metric（PE+CT=2 图），Test 2/4 各 1 图；Test 4 移除 GenerateOverview（单张多线已等价）。Test 5：FixedEstimator→ChartDim=Algorithm{Interpolate,MUSIC,ESPRIT}（新增 MUSIC/ESPRIT 两图，排除 FFT Peak）=3 图。Test 6：SNR 由 Override(−3)→ChartDim{−3,10 dB}（新增 10 dB 一图）=2 图，并修正 X 轴算法集为 Interpolate/MUSIC/ESPRIT（原 v1.7 文档误标 FFT Peak/ESPRIT）。Tests 3/7 不变。总计 23→14 图。后果：Style A（LineWithErrorBands）不再被任何测试引用（见 §8.8） |
| OQ-30 | 扫描面板完整 i18n（v1.8） | `ChartResult` 新增原子标题分量 `TestName`/`ChartDimLabel`/`IsOverview`；`SeriesResult` 新增 `PeakRank`。worker 线程只存英语 msgid（标题英文组合串仅作稳定 ID），UI 线程经 `localizedTitle()`/`localizedSeriesLabel()`/`_UI()` 本地化显示。`Peak %d` 经翻译后 `snprintf` 生成（zh_CN→"峰 N"）。⚠️ 既有事实修正（v1.8）：当时 `IMetric::name()` 各实现 `return _UI(...)`（本地化串，在 worker 线程被调用），扫描匹配沿用 `name() == _UI(metric_name)`。**v1.9 已彻底解决**（见 OQ-32）：`name()` 改为返回英语身份键，扫描匹配改为 `name() == metric_name`，worker 线程零 gettext |
| OQ-31 | 扫描图表留白与尺寸（v1.8） | 全部扫描图表 X/Y 双侧各留 5% 数据范围：`ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.1,0.1))` 包裹 `render()`（`ApplyFit` 每侧增 `(range/2)×pad`，故 0.1→每侧 5%）；作用域仅扫描面板。`plotSize()` 返回 `availableWidth − fontSize×2.5` 收窄绘图区，避免最右 X 刻度标签被子容器边框遮挡。移除 GroupedBars 原硬编码 `SetupAxisLimits(±0.6)`，改由自动拟合+FitPadding 统一处理 |
| OQ-32 | metric 身份键重构——去除 worker 线程 gettext（v1.9） | `IMetric::name()` 改为返回英语 msgid 字面量（locale 无关的身份键，兼作 gettext 翻译键），不再调用 `_UI()`。结果显示层在 UI 线程经 `_UI(name())` 本地化（原 `_UI(name())` 由"无操作双重翻译"变为正确单次翻译）。扫描测试 metric 匹配由 `name() == _UI(metric_name)` 改为 `name() == metric_name`（英语==英语），并移除 `scan_test_runner.cpp` 的 `i18n.h`。`experiment_runner` 本就不调用 `name()`/gettext。结果：worker 线程（scan/experiment runner + metric evaluate/finalize）**零** `_UI`/dgettext；`src/metrics/` 仅余 `relative_efficiency::format()` 的 `_UI("N/A")`（UI 线程）。附带安全收益：`name()` 的 `string_view` 由指向 gettext 静态缓冲区改为指向永久字面量存储。四条 metric 名 msgid 改为手工维护（xgettext 不再自动提取）。本变更落实了旧 Lesson 21 当年未真正实施的"预防性修复" |

---

## 15. 未来扩展点（非本版实现）

- **维度扫描**：把 5 个维度任意子集改造为 sweep 列表；`ExperimentRunner` 内层增加 sweep 嵌套循环，外层仍为蒙特卡洛。`ExperimentConfig` 应预留扩展形态。
- **结果导出**：CSV/JSON。
- **多算法对比**：同一配置下并列运行多种算法，结果面板表格按算法分行。
- **国际化（i18n）**：已通过 GNU gettext 实现（双文本域 `ui`/`con`，Win32 locale 自动检测，zh_CN 翻译）；不在本文档范围内，属于用户自主扩展。
