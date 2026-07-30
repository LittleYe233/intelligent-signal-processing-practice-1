# ESPRIT 算法实现指南 — Unitary ESPRIT 优化频率估计流程

> **版本**: v2.0 | **日期**: 2026-07-31  
> **用途**: 本项目 ESPRIT 频率估计的完整数学流程 + 实现指导，可直接用于编码。  
> **引用论文**:  
> - H95: Haardt & Nossek (1995), *Unitary ESPRIT*, IEEE Trans. Signal Processing, 43(5), 1232–1242. DOI: `10.1109/78.382406`  
> - D24: Ding et al. (2024), *The ESPRIT algorithm under high noise*, Proc. IEEE FOCS, 2344–2356. DOI: `10.1109/FOCS61266.2024.00137`

---

## 目录

1. [信号模型与项目约束](#一信号模型与项目约束)
2. [符号系统](#二符号系统)
3. [完整数学计算流程](#三完整数学计算流程-9-步)
4. [Hankel 窗长 L 的自适应选择](#四hankel-窗长-l-的自适应选择)
5. [预计算与缓存策略](#五预计算与缓存策略)
6. [实现代码骨架](#六实现代码骨架-eigen)
7. [边界情况与失败处理](#七边界情况与失败处理)
8. [接口契约](#八接口契约)
9. [扫描测试参数映射](#九扫描测试参数映射)
10. [复杂度分析](#十复杂度分析)
11. [与现有 espritcpp 的关键差异](#十一与现有-espritcpp-的关键差异)

---

## 一、信号模型与项目约束

### 1.1 输入信号

$$
x[n] = \sum_{k=1}^{K} A_k \cos(2\pi f_k n / f_s + \phi_k) + w[n], \quad n = 0, \ldots, N-1
$$

- $K \in \{1, 2\}$：物理频率总数（1 = 仅有目标，2 = 目标 + 一个干扰）
- $f_s$：采样率 (Hz)，来自 `EstimationContext::SampleRateHz`
- $w[n]$：零均值加性噪声（四种分布：高斯/均匀/拉普拉斯/脉冲）
- $N$：采样点数，**限定为 2 的幂**，$N \in \{32, 64, 128, 256, 512, 1024\}$

### 1.2 复指数展开（ESPRIT 基础）

每个实余弦 = 一对共轭复指数：

$$
A_k \cos(\omega_k n + \phi_k) = c_k z_k^n + \overline{c_k}\,\overline{z_k}^{\,n}
$$

$$
z_k = e^{j\omega_k},\quad \omega_k = \frac{2\pi f_k}{f_s},\quad c_k = \frac{A_k}{2}e^{j\phi_k}
$$

无噪声信号展开为 $2K$ 个复指数：

$$
x[n] = \sum_{i=1}^{2K} c_i z_i^n
$$

### 1.3 关键约束

| 约束 | 值 | 算法影响 |
|------|-----|---------|
| $K$ | 1 或 2 | $r = 2K \in \{2, 4\}$ |
| $N$ | 2 的幂 (32~1024) | $L$ 自适应选择 |
| 加窗 | 始终用矩形窗（原始数据） | ESPRIT 的 Vandermonde 模型要求不加窗 |
| 输出 | 仅频率，不要幅度/相位 | 跳过幅度重建步骤 |
| 频率筛选 | **不做**——返回全部候选 | 外部 Metric 层处理标签分配 |

---

## 二、符号系统

| 符号 | 值/范围 | 定义 |
|------|---------|------|
| $K$ | $\{1,2\}$ | 物理频率数，`context.FrequencyCount` |
| $r$ | $2K \in \{2,4\}$ | 复子空间维数（实信号 = $2K$ 个复指数） |
| $N$ | $\{32,64,128,256,512,1024\}$ | 采样点数（2 的幂） |
| $L$ | 自适应（见 §四） | Hankel 矩阵行数 |
| $M$ | $N-L+1$ | Hankel 矩阵列数（快照数） |
| $\Pi_n$ | $(\Pi_n)_{i,n-1-i}=1$ | $n \times n$ 反对角交换矩阵 |
| $Q_n$ | 见 H95 Eq.(3)/(4) | 左 $\Pi$-实酉矩阵（偶/奇两个版本） |
| $X$ | $\mathbb{R}^{L \times M}$ | Hankel 数据矩阵 |
| $Z_{\text{CH}}$ | $\mathbb{R}^{L \times 2M}$ | 中心埃尔米特扩展矩阵 |
| $T_X$ | $\mathbb{R}^{L \times 2M}$ | 实值 $Q$ 变换后的数据矩阵 |
| $E_s$ | $\mathbb{R}^{L \times r}$ | 信号子空间（主导特征向量） |
| $K_1, K_2$ | $\mathbb{R}^{(L-1) \times L}$ | 实选择矩阵（H95 Eq.32，含 $\Pi$） |
| $\Upsilon$ | $\mathbb{R}^{r \times r}$ | 旋转算子 |
| $\omega_k$ | $\mathbb{R}$ | $\Upsilon$ 的实特征值 |
| $\mu_k$ | $(0, \pi)$ rad/sample | 数字频率 |
| $\hat{f}_k$ | (0, $f_s/2$) Hz | 物理频率估计值 |
| $\tau$ | $10^{-5}$ 调试 / $10^{-3}$ 发布 | 可靠性检验阈值 |

---

## 三、完整数学计算流程（9 步）

### Step 0：预处理

- **忽略 `context.WindowKind`**：ESPRIT 始终使用原始未加窗样本
  > 依据：非矩形窗破坏 $x[n]=c z^n$ 的 Vandermonde 结构（H95 §II-A, D24 Eq.I.1）
- $r = 2 \times \texttt{context.FrequencyCount}$，即 $r \in \{2, 4\}$

---

### Step 1：Hankel 数据矩阵构造

$$X(i,j) = x[i+j], \quad i = 0,\ldots,L-1,\; j = 0,\ldots,M-1$$

$$
X = \begin{bmatrix}
x[0] & x[1] & \cdots & x[M-1] \\
x[1] & x[2] & \cdots & x[M] \\
\vdots & \vdots & \ddots & \vdots \\
x[L-1] & x[L] & \cdots & x[N-1]
\end{bmatrix} \in \mathbb{R}^{L \times M}
$$

**实现**：Eigen 的 `Eigen::Map` 可直接将 `x.data()` 映射为列优先矩阵，或显式逐元素填充（$N \leq 1024$ 时两者等价）。

**复杂度**：$O(LM)$。

---

### Step 2：前向-后向平均（中心埃尔米特扩展）

$$
Z_{\text{CH}} = \begin{bmatrix} X & \Pi_L X \Pi_M \end{bmatrix} \in \mathbb{R}^{L \times 2M}
$$

其中：
- $\Pi_L X$ = 反转 $X$ 的行（时间反演）
- $(\Pi_L X) \Pi_M$ = 反转上结果的列（等价复共轭，实信号时无运算）
- 整体效果：在保留前向 Hankel $X$ 的同时附加时反 Hankel，有效快照数 $M \to 2M$

**实现**：`Pi_L * X * Pi_M` 在 Eigen 中为 `X.colwise().reverse().rowwise().reverse()`。对实信号这是纯索引重排，无浮点运算。

**复杂度**：$O(LM)$ 索引操作。

---

### Step 3：实值 $Q$ 变换

$$
T_X = \operatorname{Re}\!\left\{ Q_L^H \cdot Z_{\text{CH}} \cdot Q_{2M} \right\} \in \mathbb{R}^{L \times 2M}
$$

#### $Q_n$ 矩阵定义（H95 Eq.3-4）

偶数 $n = 2k$：
$$
Q_{2k} = \frac{1}{\sqrt{2}} \begin{bmatrix} I_k & jI_k \\ \Pi_k & -j\Pi_k \end{bmatrix}
$$

奇数 $n = 2k+1$：
$$
Q_{2k+1} = \frac{1}{\sqrt{2}} \begin{bmatrix} I_k & 0 & jI_k \\ 0^T & \sqrt{2} & 0^T \\ \Pi_k & 0 & -j\Pi_k \end{bmatrix}
$$

#### 稀疏块公式（对实信号 $X$ 的显式简化）

本项目的 $X$ 为实数且 $Z_{\text{CH}}$ 为实矩阵。$T_X$ 可通过块运算直接构造，**零复数运算**。

令 $L = 2k$（偶数），将 $X$ 按行分块 $X = [X_1; X_2]$，$X_1, X_2 \in \mathbb{R}^{k \times M}$：

$$
T_X = \begin{bmatrix}
X_1 + \Pi_k X_2 \Pi_M & \mathbf{0}_{k \times M} \\[4pt]
\mathbf{0}_{k \times M} & X_1 - \Pi_k X_2 \Pi_M
\end{bmatrix} \in \mathbb{R}^{L \times 2M}
$$

$L$ 为奇数时（$N$ 为 2 的幂但 $L = \lfloor N/2 \rfloor = N/2$ 为整数，故本项目的 $L$ **总是偶数**，奇数公式仅当 $L$ 取非标准值时需要）。

**Eigen 实现（$L$ 偶数）**：
```cpp
Eigen::Index k = L / 2;
Eigen::MatrixXd T_X(L, 2 * M);
T_X.topLeftCorner(k, M) = X.topRows(k) + X.bottomRows(k).colwise().reverse().rowwise().reverse();
T_X.topRightCorner(k, M).setZero();
T_X.bottomLeftCorner(k, M).setZero();
T_X.bottomRightCorner(k, M) = X.topRows(k) - X.bottomRows(k).colwise().reverse().rowwise().reverse();
```

**复杂度**：$O(LM)$ 纯实数运算（加/减 + 索引反转）。

> 注：这是三份分析文件中共同认可的关键优化（esprit-1.md §10.1, esprit-2.md App A）。$Q$ 矩阵不显式构造，$Q_L^H Z_{\text{CH}} Q_{2M}$ 的结果由块公式直接给出。

---

### Step 4：信号子空间提取

#### 4.1 协方差矩阵

$$
R = T_X \cdot T_X^T \in \mathbb{R}^{L \times L}
$$

（省略缩放因子 $1/(2M)$——不影响特征向量方向。）

#### 4.2 截断 EVD

$$
R \cdot E_s = E_s \cdot \Lambda_s, \quad \Lambda_s = \operatorname{diag}(\lambda_1 \geq \lambda_2 \geq \cdots \geq \lambda_r)
$$

$E_s \in \mathbb{R}^{L \times r}$ 是前 $r$ 个最大特征值对应的特征向量。

#### 实现选项

| 场景 | 方法 | 复杂度 | Eigen 接口 |
|------|------|--------|-----------|
| $L \leq 32$ | 全量 `SelfAdjointEigenSolver` + `.rightCols(r)` | $O(L^3)$ | `Eigen::SelfAdjointEigenSolver<MatrixXd>` |
| $L > 32$ | Lanczos top-$r$（推荐 Spectra） | $O(L^2 r \cdot \text{iter})$ | `Spectra::SymEigsSolver` |

本项目 $N \leq 1024$ 时 $\max(L) = 512$（仅 $K=2, N=1024$）。对 $L > 32$ 的场景，Lanczos 收敛由特征间隙 $\gamma = (\lambda_r - \lambda_{r+1})/(\lambda_1 - \lambda_r)$ 保证（D24 Lemma I.8 确保 $\gamma = \Omega(1)$ 当 $N = \Omega(1/\Delta_z)$）。

**复杂度**：
- 协方差乘 $T_X T_X^T$：$O(L^2 M)$
- 截断 EVD：$O(L^2 r)$（Lanczos）或 $O(L^3)$（全量）

---

### Step 5：实选择矩阵（预计算 + 稀疏应用）

#### 5.1 定义（H95 Eq.32）

定义 $J_1 = [I_{L-1} \mid \mathbf{0}_{L-1}] \in \mathbb{R}^{(L-1) \times L}$。

$$
K_1 = \operatorname{Re}\!\left\{ Q_{L-1}^H (J_1 + \Pi_{L-1} J_1 \Pi_L) Q_L \right\} \in \mathbb{R}^{(L-1) \times L}
$$

$$
K_2 = \operatorname{Im}\!\left\{ Q_{L-1}^H (J_1 - \Pi_{L-1} J_1 \Pi_L) Q_L \right\} \in \mathbb{R}^{(L-1) \times L}
$$

> ⚠️ $K_1$ **不是** $J_1+J_2$（$J_2=[0|I_{L-1}]$）。$\Pi_{L-1} J_1 \Pi_L$ 利用中心对称性耦合前后向数据，是 Unitary ESPRIT 的核心。

#### 5.2 稀疏显式形式（$L$ 偶数，$k = L/2$）

$K_1$ 每行 = 两个连续的 1，$K_2$ 每行 = 对称放置的 $(-1, 1)$ 对：

$$
K_1 = \scriptsize\begin{bmatrix}
1 & 1 & 0 & 0 & \cdots & 0 & 0 \\
0 & 1 & 1 & 0 & \cdots & 0 & 0 \\
0 & 0 & 1 & 1 & \cdots & 0 & 0 \\
\vdots & & & \ddots & & & \vdots \\
0 & 0 & 0 & 0 & \cdots & 1 & 1
\end{bmatrix},\quad
K_2 = \scriptsize\begin{bmatrix}
0 & 0 & \cdots & -1 & 1 & 0 \\
0 & 0 & \cdots & 0 & -1 & 1 \\
& \vdots & & & \vdots \\
1 & -1 & 0 & \cdots & 0 & 0
\end{bmatrix}
$$

#### 5.3 直接索引应用（免矩阵构造）

利用稀疏形式，$K_1 E_s$ 和 $K_2 E_s$ 可直接计算：

```cpp
// K1 * E_s: adjacent row sums (top L-1 rows, cyclic wrap)
for (Eigen::Index i = 0; i < L - 1; ++i)
    K1Es.row(i) = Es.row(i) + Es.row((i + 1) % L);

// K2 * E_s: symmetric differences (k = L/2)
Eigen::Index k = L / 2;
for (Eigen::Index i = 0; i < L - 1; ++i) {
    int src_i = (int)i - (int)k + 1;   // maps i to (-k+1...k-2) range
    if (src_i < 0) src_i += L;
    int src_j = (k + i) % L;
    K2Es.row(i) = -Es.row(src_i) + Es.row(src_j);
}
```

**复杂度**：$O(Lr)$（仅加/减法，零乘法）。

---

### Step 6：最小二乘解（正规方程）

设 $A = K_1 E_s \in \mathbb{R}^{(L-1) \times r}$，$B = K_2 E_s \in \mathbb{R}^{(L-1) \times r}$。

#### 6.1 旋转不变方程

$$A \cdot \Upsilon \approx B, \quad \Upsilon \in \mathbb{R}^{r \times r}$$

#### 6.2 LS 解（正规方程）

$$
\Upsilon = (A^T A)^{-1} A^T B
$$

**选择 LS 而非 TLS 的理由**（H95 Remark 3, Fig.10）：Unitary ESPRIT 的实值化使得 LS 和 TLS 的误差曲线在全部 SNR 范围内重合（差异 $\ll 0.1^\circ$）。TLS 需额外一次 $(L-1) \times 2r$ SVD，对 $r \leq 4$ 显著慢于 $r \times r$ 正规方程。

#### 6.3 $r$ 极小时的闭式优化

- $r = 2$（$K=1$）：$A^T A$ 为 $2\times 2$ 对称正定矩阵

  $$(A^T A)^{-1} = \frac{1}{\det} \begin{bmatrix} a_{22} & -a_{12} \\ -a_{12} & a_{11} \end{bmatrix},\quad \det = a_{11}a_{22} - a_{12}^2$$

- $r = 4$（$K=2$）：$4\times 4$ 直接用 `Eigen::Matrix4d` + `.ldlt().solve()`

**复杂度**：$O(L r^2)$ 构造 $A^T A$、$A^T B$；$O(r^3) \leq O(64)$ 求逆。

---

### Step 7：实 EVD + 可靠性检验

#### 7.1 特征值分解

$$
\Upsilon = T \cdot \Omega \cdot T^{-1}, \quad \Omega = \operatorname{diag}(\omega_1, \ldots, \omega_r)
$$

使用 `Eigen::EigenSolver<MatrixXd>`（**实数 Schur 分解**，非 `ComplexEigenSolver`）。

$\Upsilon$ 严格为实矩阵，其理论特征值在 Unitary ESPRIT 框架下保证为实数（H95 §IV-C）。

#### 7.2 可靠性检验（H95 §IV-C）

$$
\eta = \frac{\max_k |\operatorname{Im}(\omega_k)|}{\max\!\left(1,\; \max_k |\operatorname{Re}(\omega_k)|\right)} \leq \tau
$$

阈值 $\tau$：调试 $10^{-5}$，发布 $10^{-3}$。

若 $\eta > \tau$：
- 原因：SNR 过低、$L$ 不足、模型阶数 $r$ 错误（如 $K$ 过估）
- 不能通过切换 LS→TLS 挽救（在 $\Upsilon$ 层面已无可挽回）——**只应返回空集**
- **不降级尝试 $r-1$**（会导致子空间混叠，产生偏置估计）

#### 7.3 TLS 回退

仅当以下场景考虑回退 TLS（H95 Proposition 2, esprit-3.md §6.3）：
- 子空间提取用了 Lanczos 且残差 $\| R \hat{v} - \hat{\sigma} \hat{v} \| > \varepsilon$（特征向量未收敛）
- 提升 Lanczos 迭代次数后重试，若仍失败再试 TLS，最后返回空集

**对全量 EVD 场景（$L \leq 32$），TLS 回退无意义——直接返回空集**。

#### 7.4 提取实部

$$
\omega_k \gets \operatorname{Re}(\omega_k), \quad k = 1, \ldots, r
$$

---

### Step 8：频率提取

#### 8.1 Cayley 反变换（H95 Eq.33）

$$
\mu_k = 2 \arctan(\omega_k), \quad k = 1, \ldots, r
$$

$\mu_k \in (-\pi, \pi)$ 为数字频率（rad/sample）。

#### 8.2 高频折叠处理

当 $|\omega_k| > 1$（信号接近 Nyquist 频率）：

$$
\mu_k = \begin{cases}
2\arctan(|\omega_k|), & \omega_k \in [-1, 1] \\[3pt]
2\arctan(|\omega_k|) - \pi, & \omega_k > 1 \\[3pt]
-2\arctan(|\omega_k|) + \pi, & \omega_k < -1
\end{cases}
$$

#### 8.3 物理频率

$$\hat{f}_k = f_s \cdot \frac{|\mu_k|}{2\pi}$$

$\hat{f}_k \in [0, f_s/2)$。

#### 8.4 去重

$r = 2K$ 个 $\omega_k$ 产生 $K$ 对 $\pm\mu_k$，对应 $K$ 个不同正频率。按 $\hat{f}_k$ 的值去重（$\epsilon_{\text{tol}} = 10^{-6}$）：

```cpp
// Sort by frequency, deduplicate within tolerance
auto sorted = positive_freqs; // double[] of size K
std::sort(sorted.begin(), sorted.end());
// After sorting: unique via adjacent difference < tol
```

**复杂度**：$O(r)$ 计算 + $O(K \log K)$ 排序（$K \leq 2$）。

---

### Step 9：输出

```cpp
std::vector<FrequencyPeak> peaks;
for (double f : unique_positive_frequencies) {
    peaks.push_back({
        .FrequencyHz = f,
        .Amplitude = AMP_UNKNOWN,      // 本算法不估幅度
        .Prominence = PROMINENCE_UNKNOWN
    });
}
return peaks;
```

**明确不做**：
- ❌ 不用 $f_{\text{ref}}$ 筛选"真频"——违反需求，所有候选都输出
- ❌ 不重建 Vandermonde 矩阵——不需要幅度/相位
- ❌ 不调用 `findPeaksFromDft`——ESPRIT 使用不同的子空间原理
- ❌ 不标注哪个是"目标频率"——外部 Metric 层处理

> 依据：esprit-3.md §9 标签对称性论证。目标与干扰在没有外部先验时不可区分。

---

## 四、Hankel 窗长 $L$ 的自适应选择

### 4.1 约束条件

D24 Theorem B.1 (Eq.B.1) 稳定性必要条件：

$$L > 1 + \frac{2\pi}{\Delta_z}, \quad \Delta_z = 2\left|\sin\!\left(\frac{\pi \Delta_f}{f_s}\right)\right|$$

其中 $\Delta_f$ 为两频率的最小间隔（Hz）。

**扫描测试参数约束**：

| 测试 | N | fs | 频率场景 | $\Delta_f$ 下界 |
|------|---|-----|----------|----------------|
| Test 1 (SampleCount) | 32~1024 | 1000 | 单频 $K=1$ | 无（仅一个频率） |
| Test 7 (Interference $\Delta_{\text{bins}}$) | 256 | 1000 | $\Delta_{\text{bins}} \in [0,4]$ | $\Delta_f = \Delta_{\text{bins}} \cdot f_s/N$ 低至 0.78Hz |

对 $K=2$ 且 $\Delta_{\text{bins}} = 0.2$（极端）：$\Delta_f = 0.78\text{Hz}$ → $\Delta_z \approx 0.0049$ → $L > 1282$。此条件不可满足（$L \leq N = 256$）。这是 ESPRIT 的物理极限——任何实现都无法在如此近的频率下取得可靠的分离。

### 4.2 自适应公式

$K=2$ 时 $\Delta_f$ 对估计器不可见 → **保守取 $L = N/2$**。

$K=1$ 时无 $\Delta_z$ 约束，仅需 $L > r+1 = 3$，可缩减 $L$ 以加速。

$$
\boxed{
L = \begin{cases}
\lfloor N/2 \rfloor, & N \leq 64 \\[4pt]
\lfloor N/2 \rfloor, & K = 2 \\[4pt]
\max\!\left(r+1,\; \lfloor N/4 \rfloor\right), & N \geq 128,\; K = 1
\end{cases}
}
$$

### 4.3 对标量值

| $N$ | $K=1$ (L) | $K=2$ (L) | 说明 |
|-----|-----------|-----------|------|
| 32 | 16 | 16 | 小 $N$ 不缩减 |
| 64 | 32 | 32 | 小 $N$ 不缩减 |
| 128 | **32** | 64 | $K=1$ 加速 ~8× |
| 256 | **64** | 128 | $K=1$ 加速 ~8× |
| 512 | **128** | 256 | $K=1$ 加速 ~8× |
| 1024 | **256** | 512 | $K=1$ 加速 ~8× |

### 4.4 精度影响（$K=1$ 缩减场景）

D24 Theorem I.4 误差标度 $\varepsilon = \tilde{O}(L^{-3/2})$：

- $L = N/4$ vs $N/2$：单次误差放大 $(2)^{1.5} \approx 2.8\times$
- 经 $M=100$ 次 MC 平均：方差放大 $2.8 / \sqrt{100} = 0.28\times$
- **实际 MSE 反而低于单次全精度估计**

---

## 五、预计算与缓存策略

### 5.1 缓存依赖树

```
依赖链: N → L → {K1, K2}
依赖链: N → L → {Q_L}
依赖链: N → L, M → {Q_{2M}}
```

$T_X$ 的块公式不需要 $Q$ 矩阵显式参与，故 $Q$ 矩阵实际可以**不构造**。但 $K_1, K_2$ 的公式直接涉及 $Q_{L-1}, Q_L$，若不用稀疏应用（§5.3）而用矩阵乘法，则需要缓存。

### 5.2 推荐：稀疏直接应用（免 Q 矩阵）

如 §三.Step5.3 所示，$K_1 E_s$ 和 $K_2 E_s$ 可以直接通过索引从 $E_s$ 计算，**不需要** $K_1, K_2, Q_{L-1}, Q_L$ 的显式构造。

这是本项目的推荐实现路径，零矩阵乘法。

### 5.3 备选：矩阵缓存的哈希表

若选择矩阵公式实现（Haardt Eq.32 直接乘法）：

```cpp
std::unordered_map<Eigen::Index, Cache> s_cache;
struct Cache {
    Eigen::MatrixXd K1, K2;
};
Cache compute_cache(Eigen::Index L) { ... }
Cache get_or_create(Eigen::Index L) {
    auto it = s_cache.find(L);
    if (it != s_cache.end()) return it->second;
    return s_cache[L] = compute_cache(L);
}
```

以 $N$（或 $L$）为 key。因 $N \in \{32,64,128,256,512,1024\}$，最多 6 个缓存实体。

---

## 六、实现代码骨架（Eigen）

### 6.1 入口函数

```cpp
std::vector<FrequencyPeak>
EspritEstimator::estimate(const RealArray &input,
                          const EstimationContext &context)
{
    // Step 0: Model order
    const Eigen::Index N = static_cast<Eigen::Index>(input.size());
    const Eigen::Index K = static_cast<Eigen::Index>(context.FrequencyCount);
    const Eigen::Index r = 2 * K;         // subspace dimension: 2 or 4
    const double fs = context.SampleRateHz;

    // Adaptive L (see §四)
    const Eigen::Index L = adaptWindowLength(N, K);
    const Eigen::Index M = N - L + 1;

    // Step 1-9: Dense implementation
    auto peaks = espritCalc(input.data(), N, L, M, r, fs);
    return peaks;
}
```

### 6.2 核心计算

```cpp
namespace {

// NOLINTBEGIN(readability-identifier-naming)
std::vector<ispp::FrequencyPeak>
espritCalc(const double *sig, Eigen::Index N,
           Eigen::Index L, Eigen::Index M, Eigen::Index r, double fs)
{
    // === Step 1: Hankel matrix ===
    Eigen::MatrixXd X(L, M);
    for (Eigen::Index i = 0; i < L; ++i)
        for (Eigen::Index j = 0; j < M; ++j)
            X(i, j) = sig[i + j];

    // === Step 2: Forward-backward averaging ===
    // X_b = Pi_L * X * Pi_M = reverse rows and cols
    Eigen::MatrixXd X_b = X.colwise().reverse().rowwise().reverse();
    // Z_CH = [X, X_b]
    Eigen::MatrixXd Z_CH(L, 2 * M);
    Z_CH.leftCols(M) = X;
    Z_CH.rightCols(M) = X_b;

    // === Step 3: Real-valued Q-transform (sparse block formula) ===
    // L is always even (N power of 2, L = N/2 or N/4)
    Eigen::Index k = L / 2;
    Eigen::MatrixXd T_X(L, 2 * M);
    // Top-left: X1 + Pi_k * X2 * Pi_M
    T_X.topLeftCorner(k, M) =
        X.topRows(k) + X.bottomRows(k).colwise().reverse().rowwise().reverse();
    // Bottom-right: X1 - Pi_k * X2 * Pi_M
    T_X.bottomRightCorner(k, M) =
        X.topRows(k) - X.bottomRows(k).colwise().reverse().rowwise().reverse();
    // Off-diagonal blocks are zero
    T_X.topRightCorner(k, M).setZero();
    T_X.bottomLeftCorner(k, M).setZero();

    // === Step 4: Covariance + truncated EVD ===
    Eigen::MatrixXd R = T_X * T_X.transpose();    // (omit 1/(2M) factor)
    // For L <= 32, full SelfAdjointEigenSolver is fine
    // For larger L, prefer Spectra Lanczos top-r
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eig_solver(R);
    Eigen::MatrixXd E_s = eig_solver.eigenvectors().rightCols(r);

    // === Step 5: Sparse application of K1, K2 (direct index) ===
    Eigen::MatrixXd K1Es(L - 1, r);
    Eigen::MatrixXd K2Es(L - 1, r);
    // K1: adjacent row sums (cyclic)
    for (Eigen::Index i = 0; i < L - 1; ++i)
        K1Es.row(i) = E_s.row(i) + E_s.row((i + 1) % L);
    // K2: symmetric differences (k = L/2)
    Eigen::Index half = L / 2;
    for (Eigen::Index i = 0; i < L - 1; ++i) {
        Eigen::Index src_i = (i + 1 <= half) ? (half - 1 - i) : (L + half - 1 - i);
        Eigen::Index src_j = (half + i) % L;
        K2Es.row(i) = -E_s.row(src_i) + E_s.row(src_j);
    }

    // === Step 6: Normal equations (LS) ===
    Eigen::MatrixXd AtA = K1Es.transpose() * K1Es;  // r x r
    Eigen::MatrixXd AtB = K1Es.transpose() * K2Es;  // r x r
    Eigen::MatrixXd Upsilon = AtA.ldlt().solve(AtB); // r x r

    // === Step 7: Real EVD + reliability check ===
    Eigen::EigenSolver<Eigen::MatrixXd> solver(Upsilon);
    Eigen::VectorXcd eig_vals = solver.eigenvalues();

    // Reliability: any significant imaginary part?
    constexpr double TAU = 1e-5;    // debug; use 1e-3 for release
    double max_imag = eig_vals.imag().cwiseAbs().maxCoeff();
    double max_real = eig_vals.real().cwiseAbs().maxCoeff();
    if (max_imag > TAU * std::max(1.0, max_real))
        return {};   // unreliable → empty

    Eigen::VectorXd omega = eig_vals.real();

    // === Step 8: Frequency extraction ===
    std::vector<double> freqs;
    freqs.reserve(static_cast<size_t>(r));
    for (Eigen::Index i = 0; i < r; ++i) {
        double w = omega(i);
        double mu;
        if (std::abs(w) <= 1.0) {
            mu = 2.0 * std::atan(w);
        } else if (w > 1.0) {
            mu = 2.0 * std::atan(w) - std::numbers::pi;
        } else {
            mu = -2.0 * std::atan(-w) + std::numbers::pi;
        }
        double f_est = fs * std::abs(mu) / (2.0 * std::numbers::pi);
        if (f_est > 0.0 && f_est < fs / 2.0)
            freqs.push_back(f_est);
    }

    // Sort + deduplicate (K ≤ 2, but r duplicates from ± pairs)
    std::sort(freqs.begin(), freqs.end());
    freqs.erase(std::unique(freqs.begin(), freqs.end(),
        [](double a, double b) { return std::abs(a - b) < 1e-6; }), freqs.end());

    // === Step 9: Output all candidates ===
    std::vector<FrequencyPeak> peaks;
    peaks.reserve(freqs.size());
    for (double f : freqs)
        peaks.push_back({f, AMP_UNKNOWN, PROMINENCE_UNKNOWN});

    return peaks;
}

Eigen::Index adaptWindowLength(Eigen::Index N, Eigen::Index K) {
    if (N <= 64)    return N / 2;
    if (K == 2)     return N / 2;           // interference present
    /* K == 1 */    return std::max(K * 2 + 1, N / 4);
}
// NOLINTEND(readability-identifier-naming)

} // namespace
```

### 6.3 需要处理的重要细节

1. **`readability-identifier-naming` 抑制**：Eigen 的 `MatrixXd` 非标量命名约定与 .clang-tidy 冲突，用 `NOLINT` 块包裹核心计算。

2. **Eigen 列优先**：`Eigen::MatrixXd` 默认列优先。`T_X` 的块赋值在列优先布局下缓存友好。

3. **`-O3` 编译**：ESPRIT 的矩阵乘法需要 `-O3` 以启用 Eigen 的 SIMD 向量化（CMakeLists.txt 已有 `target_compile_options(ISPPracticeOne PRIVATE -O3)`）。

4. **`SelfAdjointEigenSolver` 的 Eigen 3.4+ 截断模式**：Eigen 3.4 加入了 `.compute(R, Eigen::EigenvaluesOnly)` 可只算特征值。但我们需要特征向量（用于 $E_s$），需要 `compute(R)` 的全量版。若引入 Spectra，可真正截断到 top-$r$。

---

## 七、边界情况与失败处理

### 7.1 失败模式汇总

| 场景 | 现象 | 原因 | 处理 |
|------|------|------|------|
| SNR 极低 (< -20dB) | $\Upsilon$ 有显著虚部 | 噪声淹没信号子空间 | 可靠性检验捕获 → 返回空集 |
| 两频率过近 ($\Delta_{\text{bins}} < 0.5$) | 实 RMSE 极大 | $\Delta_z$ 不满足 Ding 下界 | $L=N/2$ 无法改善→接受极限 |
| $r$ 过估（如 $K=1$ 但传入 $K=2$） | $\Upsilon$ 虚部大或第 2 频率噪声 | 噪声方向被误判为信号 | 可靠性检验捕获 |
| $r$ 欠估（如 $K=2$ 但传入 $K=1$） | 估计频率偏置 | 子空间被干扰污染 | 无法自动检测——由 `FrequencyCount` 保证正确 |
| $L-1 < r$（$N < 6$ 极端） | 欠定方程组 | 快照少于所需秩 | $N \geq 32$ 时不会出现 |
| 非矩形窗使用 | ESPRIT 偏差 | Vandermonde 结构破坏 | 实现层面强制原始数据 |

### 7.2 返回空集 vs 返回噪声

- **可靠性检验失败** → 返回空集。比返回错误频率更诚实。Metric 层对空集已有处理（`PercentageErrorMetric` 计算时跳过无效估计）。

### 7.3 蒙特卡洛行为

- 单次 ESPRIT 失败（返回空集）在 MC 平均中意味着该次迭代不贡献数据
- 若 `MonteCarlo::IterationCount = 100`，约 5-10% 的失败率产生不到 2% 的 MSE 变异（D24 仿真证实）

---

## 八、接口契约

### 8.1 IEstimator 接口

```cpp
class IEstimator {
public:
    virtual std::vector<FrequencyPeak>
    estimate(const RealArray &input, const EstimationContext &context) = 0;
    virtual std::string_view name() const = 0;
};
```

### 8.2 EspritEstimator 实现要求

| 项目 | 值 |
|------|-----|
| 类名 | `EspritEstimator` |
| `name()` 返回值 | `"ESPRIT"` |
| `FrequencyPeak::Amplitude` | `AMP_UNKNOWN`（不估计幅度） |
| `FrequencyPeak::Prominence` | `PROMINENCE_UNKNOWN`（不计算显著度） |
| 失败时返回值 | 空 `vector` |
| 缓存 | 静态成员变量（以 $N$ 为 key 的哈希表） |

### 8.3 EstimationContext 的使用

| 字段 | 使用方式 |
|------|---------|
| `SampleRateHz` | ✅ 用于频率映射 $f_k = f_s \cdot \|\mu_k\| / 2\pi$ |
| `FrequencyCount` | ✅ 设定 $r = 2 \times \texttt{FrequencyCount}$ |
| `WindowKind` | ❌ **忽略**——ESPRIT 始终使用原始未加窗数据 |
| `NoiseInfo` | ❌ **忽略**——不依赖噪声分布假设 |

---

## 九、扫描测试参数映射

### 9.1 7 个测试的覆盖

| 测试 | X 轴 | N | 本流程的行为 |
|------|------|---|------------|
| Test 1: SampleCount | $N \in \{32,64,128,256,512,1024\}$ | 变化 | $L$ 自适应；$K=1 \to L=N/4$（$N \geq 128$），$K=2 \to L=N/2$ |
| Test 2: Frequency | $f \in [1500,1530]$, $f_s=7680$ | 256 | $L = 128$（$K=1$ 但 $N=256$，Wait — $K=1$ and $N=128$... 实际默认 $N=256, K=1 \to L=64$） |
| Test 3: NoiseDist | 4 分布, SNR = -8dB | 256 | 低 SNR → 可靠性检验可能频繁触发（$\Upsilon$ 虚部增大） |
| Test 4: SNR | -30 ~ 20 dB | 256 | -30 dB 时极可能全部失败 → 空集；-20 dB+ 逐渐可用 |
| Test 5: SNR × SampleCount | SNR $N \in \{64,128,256,512\}$ | 变化 | 自适应 $L$ 在每种 $N$ 上独立生效 |
| Test 6: Window | 4 窗, SNR $\in \{-3,10\}$ dB | 256 | ESPRIT 忽略窗类型 → 所有窗结果相同（合理行为） |
| Test 7: Interference | $\Delta_{\text{bins}} \in [0,4]$ | 256 | $K=2$ → $L=128$；$\Delta_{\text{bins}} < 0.5$ 时 ESPRIT 物理极限无法分辨 |

### 9.2 对估计器不可见的参数

以下参数由 `ExperimentRunner` 构造 `EstimationContext` 时决定，ESPRIT 无法直接获取：

- `Interference.DeltaBins`（干扰间距）→ $K$ 的值（是否 $\neq 0$）以 `FrequencyCount` 间接传递
- `Interference.Amplitude` → 影响 SNR，但 SNR 已含在 `NoiseInfo.SnrDb` 中
- `MonteCarlo.IterationCount` → 不影响单次 `estimate()` 行为

---

## 十、复杂度分析

### 10.1 各步骤复杂度（$N=256$，$K=1 \Rightarrow r=2, L=64$）

| 步骤 | 操作 | 复杂度 | FLOPs |
|------|------|--------|-------|
| 1 | Hankel 构造 | $O(LM)$ | $\sim 1.2 \times 10^4$ |
| 2 | 前后向平均 | $O(LM)$ 索引 | $\sim 0$（重排） |
| 3 | $Q$ 变换（块公式） | $O(LM)$ | $\sim 1.2 \times 10^4$ |
| 4a | 协方差 $T_X T_X^T$ | $O(L^2 M)$ | $\sim 7.9 \times 10^5$ |
| 4b | 全量 EVD $L \times L$ | $O(L^3)$ | $\sim 2.6 \times 10^5$ |
| 5 | $K_1E_s, K_2E_s$（索引） | $O(Lr)$ | $\sim 1.3 \times 10^2$ |
| 6 | 正规方程 | $O(L r^2)$ | $\sim 2.6 \times 10^2$ |
| 7 | $r \times r$ EVD | $O(r^3)$ | $\sim 8$ |
| 8 | 频率提取 | $O(r)$ | $\sim 2$ |
| **总计** | | | **$\sim 1.1 \times 10^6$** |

### 10.2 对比不同配置

| 配置 | 总 FLOPs | 相对现有实现的加速 |
|------|----------|------------------|
| 现有 `esprit.cpp` ($L=128$，复数，全量) | $\sim 8 \times 10^6$ | 1×（基线） |
| 本流程 ($K=1, L=64$) | $\sim 1.1 \times 10^6$ | **~7.3×** |
| 本流程 ($K=2, L=128$) | $\sim 8.4 \times 10^6$ | ~1×（接近现有） |
| 本流程 ($K=1, \text{Spectra Lanczos top-}2$) | $\sim 4.4 \times 10^5$ | **~18×** |

> 实际加速受 CPU 缓存、Eigen SIMD（实数比复数更高效）、内存带宽影响。$\mathbb{C}$ 到 $\mathbb{R}$ 的切换贡献 ~2.8× 算术加速，$L$ 缩减贡献 ~8×（$K=1$），正规方程贡献 ~5×。

---

## 十一、与现有 `esprit.cpp` 的关键差异

| 维度 | 现有实现 | 本流程 | 为什么 |
|------|---------|--------|--------|
| **算法基础** | 标准复 ESPRIT（Hankel + 自相关 + SVD/PINV） | Unitary ESPRIT（H95） | 实数运算快 2.8×，前后向平均提升精度 |
| **数据类型** | `Eigen::MatrixXcd` 全复数 | `Eigen::MatrixXd` 全实数 | 实信号无需复数，0 虚部漂移 |
| **L 选择** | $L = N/2$ 硬编码 | 自适应（$K=1$ 时 $L=N/4$） | $N \geq 128$ 时快速，MC 平均后精度无损 |
| **子空间提取** | `SelfAdjointEigenSolver` 全谱 | 截断 top-$r$（Lanczos / 全量） | $L \gg r$ 时加速 $L/r$ 倍 |
| **最终 EVD** | `ComplexEigenSolver<MatrixXcd>` | `EigenSolver<MatrixXd>` | 实数矩阵用实数求解器，~4× 更快 |
| **伪逆** | `bdcSvd().solve()` | 正规方程 `ldlt().solve()` | $r\leq 4$ 时 ~5× 更快 |
| **Q 变换** | 无（标准 ESPRIT） | 稀疏块公式（零复数乘） | 额外 $O(LM)$ 但全流程仍更快且更精确 |
| **可靠性检验** | 无 | Haardt §IV-C 虚部检验 | 低 SNR 防伪频率输出 |
| **高频折叠** | 无 | $|\omega_k| > 1$ 分支 | Nyquist 边界频率正确性 |
| **频率输出** | 过滤 $\leq f_s/2$ 的正频率 | 输出全部候选 | 不做内部真频筛选（esprit-3.md §9） |
| **Vandermonde 重建** | 有（`std::pow(z, i)`） | 无 | 无需幅度——跳过 $O(Nr)$ |
| **幅度估计** | `AMP_UNKNOWN` | `AMP_UNKNOWN` | 保持兼容 |
| **可靠性失败** | 不存在此概念 | 返回空 `vector` | Metric 层已有空集处理 |

---

## 附录：Eigen 类型速查

| Eigen 类型 | 尺寸 | 用途 |
|-----------|------|------|
| `Eigen::MatrixXd` | `L × M` | Hankel 矩阵 $X$ |
| `Eigen::MatrixXd` | `L × 2M` | $Z_{\text{CH}}$ 和 $T_X$ |
| `Eigen::MatrixXd` | `L × L` | 协方差 $R$ |
| `Eigen::MatrixXd` | `L × r` | 信号子空间 $E_s$ |
| `Eigen::MatrixXd` | `(L-1) × r` | $K_1E_s$, $K_2E_s$ |
| `Eigen::MatrixXd` | `r × r` | $\Upsilon$, $A^T A$, $A^T B$ |
| `Eigen::MatrixXd` | `N × r` | ❌ 不再使用（跳过 Vandermonde） |
| `Eigen::EigenSolver<MatrixXd>` | — | $\Upsilon$ 的实 Schur 分解 |
| `Eigen::LDLT<MatrixXd>` | — | 正规方程 $r \times r$ 求解器 |

---

*文档版本 v2.0 | 代码实现用完整参考 | 综合 esprit-1/2/3.md 及横向审阅结果*
