# ESPRIT 算法优化计算流程 — 面向实正弦信号的频率估计

> **编写日期**: 2026-07-31  
> **对应论文**:  
> 1. **Ding, Epperly, Lin & Zhang (2024)** — *The ESPRIT algorithm under high noise: Optimal error scaling and noisy super-resolution*. IEEE FOCS 2024. DOI: `10.1109/FOCS61266.2024.00137` (arXiv: `2404.03885`)  
>    关注点: 噪声条件下 ESPRIT 的 $\tilde{\mathcal{O}}(n^{-3/2})$ 最优误差缩放的严格证明。  
> 2. **Haardt & Nossek (1995)** — *Unitary ESPRIT: How to obtain increased estimation accuracy with a reduced computational burden*. IEEE Trans. Signal Processing, Vol. 43, No. 5. DOI: `10.1109/78.382406`  
>    关注点: 实值化变换、前后向平均、可靠性检验、LS vs TLS 分析。  

---

## 1. 项目上下文与本文件目的

本项目（`ISPPracticeOne`）当前已有一个基于 Unitary ESPRIT 的 ESPRIT 频率估计实现（`src/estimator/esprit.cpp`），该实现使用了 Eigen 线性代数库（`SelfAdjointEigenSolver` + `ComplexEigenSolver`），采用了 Hankel 数据矩阵 + 中心埃尔米特扩展 + Q 变换的框架。

本文件旨在 **从两篇参考论文的理论出发，严格推导出一个针对本项目特定需求优化的 ESPRIT 计算流水线数学公式**。项目特定需求为：

- **输入**：实数采样序列（单频正弦信号，最多含一个干扰 = 最多 2 个频率分量）
- **噪声**：四种加性噪声分布（高斯 / 均匀 / 拉普拉斯 / 脉冲）
- **输出**：仅需真实频率（丢弃干扰频率）
- **约束**：最大化计算效率，且不引入过度的精度损失
- **N 范围**：32 ~ 1024（功率 2 的倍数，由测试规格得出）

---

## 2. 两篇论文的核心启示

### 2.1 Ding et al. (2024) — FOCS

| 方面 | 内容 |
|---|---|
| **主要贡献** | 证明 ESPRIT 在噪声条件下的最优误差缩放 $\epsilon = \tilde{\mathcal{O}}(n^{-3/2})$，并提供严格下界证明 $\epsilon = \Omega(n^{-3/2})$（定理 I.7 / III.1） |
| **对优化的意义** | ① 证明了使用正确子空间维数 $r$ 时，ESPRIT **不会**因优化数值方法而损失渐近精度；② 提供二阶特征空间摄动展开（引理 I.9 / IV.1）作为子空间估计的理论基础；③ 论文使用的 **Toeplitz 矩阵版本** 与本项目的 Hankel 版本可互换——但本项目当前实现已采用 Hankel，理论上等价 |
| **关键不等式** | 定理 I.2（中心极限缩放）：$\text{md}(\hat{z}, z) = \mathcal{O}\!\left(\frac{\mu_{\text{tail}}}{\mu_r \Delta_z n} + \frac{\alpha\sqrt{\log n}}{\mu_r\sqrt{n}}\right)$；定理 I.4（最优缩放）：$\text{md}(\hat{z}, z) = \tilde{\mathcal{O}}\!\left(\frac{r^{1.5}\alpha^3}{\mu_r^3 \Delta_z^{1.5} n^{1.5}}\right)$ |
| **对本项目的约束** | 需要频率分离条件 $\Delta_z > 0$（真频与干扰频率充分分开）；噪声子高斯尾。我们的四种噪声均满足或近似满足 |

### 2.2 Haardt & Nossek (1995) — Unitary ESPRIT

| 方面 | 内容 |
|---|---|
| **主要贡献** | 将 ESPRIT 的所有复数计算映射为等价实值计算（Lee 变换），同时前-后向平均使有效数据量加倍、提高精度 |
| **对优化的意义** | ① **实值化**：所有复矩阵运算替换为实矩阵，运算量减半；② **可靠性检验**：$\Upsilon$ 的特征值必为实数（或共轭对）——可做快速故障检测；③ **LS ≈ TLS**（Fig. 10 实证）：省去 TLS 的额外 SVD；④ **$\Upsilon$ 的 EVD 实值化**：式 (27) 将最后的复特征分解转为实特征分解 |
| **关键公式** | $Q_n$ 矩阵定义（式 3-4）；$K_1, K_2$ 实选择矩阵（式 32）；$\mu_k = 2\arctan(\omega_k)$（式 33）；LS 解的 $T_1\Upsilon \approx T_2$（注记 3） |
| **对本项目的价值** | 本项目已有 Unitary ESPRIT 实现框架，但有多处可优化的"低垂果实"（见第 6 节） |

---

## 3. 问题建模与符号系统

### 3.1 信号模型

$$
x[n] = \sum_{k=1}^{K} A_k \cos\!\big(2\pi f_k n/f_s + \phi_k\big) + \varepsilon[n], \quad n = 0, 1, \dots, N-1
$$

- $K \in \{1, 2\}$：$K=1$ 仅有真频，$K=2$ 有真频 + 干扰
- $f_s$：采样率 (Hz)
- $\varepsilon[n]$：加性噪声（满足 $\mathbb{E}[\varepsilon] = 0$）
- **已知量**：$N$, $f_s$, $K$（通过 `EstimationContext::FrequencyCount` 传入）

### 3.2 复指数映射

$$
x[n] = \sum_{i=1}^{2K} c_i \cdot z_i^n + \varepsilon[n]
$$

其中：

- $z_i = e^{j\omega_i} \in \mathbb{T}$，$\omega_i = 2\pi f_i/f_s$（$i=1,\dots,K$）
- $\omega_{K+i} = -\omega_i$（共轭对），$c_{K+i} = c_i^*$
- $r = 2K$：复子空间维数（$r=2$ 或 $r=4$）

### 3.3 符号汇总

| 符号 | 含义 | 典型值 |
|---|---|---|
| $N$ | 采样点数 | 32, 64, 128, 256, 512, 1024 |
| $L$ | Hankel 矩阵行数 $= \lfloor N/2 \rfloor$ | 16 ~ 512 |
| $M$ | Hankel 矩阵列数 $= N-L+1$ | $L$ 或 $L+1$ |
| $r$ | 子空间维数 $= 2K$ | 2 或 4 |
| $\Pi_n$ | $n \times n$ 交换矩阵（反单位阵） | — |
| $Q_n$ | 左 $\Pi$-实酉矩阵（式 (3)/(4)） | — |
| $K_1, K_2$ | 实选择矩阵（式 (32)） | $(L-1) \times L$ |
| $\Upsilon$ | 旋转算子 $r \times r$ 实矩阵 | $2 \times 2$ 或 $4 \times 4$ |

---

## 4. 完整数学计算流水线

### Step 1: Hankel 数据矩阵构造

$$
X = \begin{bmatrix}
x[0] & x[1] & \cdots & x[M-1] \\
x[1] & x[2] & \cdots & x[M] \\
\vdots & \vdots & \ddots & \vdots \\
x[L-1] & x[L] & \cdots & x[N-1]
\end{bmatrix} \in \mathbb{R}^{L \times M}
$$

其中 $L = \lfloor N/2 \rfloor$，$M = N - L + 1$。

**复杂度**: $O(LM)$。此步骤已为最优，无需优化。

---

### Step 2: 前后向平均 → 中心埃尔米特扩展

$$
Z_{\text{CH}} = \big[\, X \;\; \Pi_L X \Pi_M \,\big] \in \mathbb{R}^{L \times 2M}
$$

其中：

$$
\Pi_n = \begin{bmatrix}
0 & \cdots & 0 & 1 \\
0 & \cdots & 1 & 0 \\
\vdots & \cdot\cdot & \vdots & \vdots \\
1 & 0 & \cdots & 0
\end{bmatrix} \in \mathbb{R}^{n \times n}
$$

**物理意义**: $\Pi_L X \Pi_M$ 对 $X$ 同时反转行（时间反演）和列（等同复共轭），等效于前-后向平均。

**关键性质**: $Z_{\text{CH}}$ 是中心埃尔米特矩阵：$\Pi_L \overline{Z}_{\text{CH}} \Pi_{2M} = Z_{\text{CH}}$。对于实信号，$\overline{Z}_{\text{CH}} = Z_{\text{CH}}$，因此兼有中心对称性。

---

### Step 3: 实值化 Q 变换

#### 3.1 Q 矩阵定义（奇数/偶数统一公式）

**偶数 $n = 2k$**:

$$
Q_n = \frac{1}{\sqrt{2}} \begin{bmatrix} I_k & j I_k \\ \Pi_k & -j \Pi_k \end{bmatrix}, \quad Q_n Q_n^H = I_n
$$

**奇数 $n = 2k+1$**:

$$
Q_n = \frac{1}{\sqrt{2}} \begin{bmatrix} I_k & 0 & j I_k \\ 0^T & \sqrt{2} & 0^T \\ \Pi_k & 0 & -j \Pi_k \end{bmatrix}
$$

#### 3.2 实值映射（Haardt 定理 1）

$$
T_X = \operatorname{Re}\!\big\{ Q_L^H \cdot Z_{\text{CH}} \cdot Q_{2M} \big\} \in \mathbb{R}^{L \times 2M}
$$

Haardt & Nossek 定理 1 保证：由于 $Z_{\text{CH}}$ 是中心埃尔米特的，$Q_L^H Z_{\text{CH}} Q_{2M}$ 的虚部严格为零；实际计算中的 `.real()` 仅消除数值噪声。

**优化要点**：
- $Q_L$, $Q_{2M}$ **在给定 $N$ 后为常量**，应预计算并缓存
- $T_X$ 的显式块矩阵公式（文献式 (7)）可替代复数乘法，将 $O(L^2 M)$ 降为 $O(LM)$。但对于本项目 $N \leq 1024$ 的规模，复数乘法已足够快

---

### Step 4: 信号子空间提取

#### 4.1 协方差矩阵

$$
R = T_X \cdot T_X^T \in \mathbb{R}^{L \times L}
$$

> **注**: 缩放因子 $1/(2M)$ 不影响特征向量，可省略以节省 $O(L^2)$。

#### 4.2 实对称 EVD — 仅提取 top-$r$ 特征向量

$$
R = E \cdot \Sigma \cdot E^T, \quad \Sigma = \operatorname{diag}(\sigma_1 \geq \sigma_2 \geq \cdots \geq \sigma_L \geq 0)
$$

信号子空间：

$$
E_s = E[:, \; L-r : L] \in \mathbb{R}^{L \times r}, \quad r = 2K \in \{2, 4\}
$$

**关键优化 —— Lanczos 迭代**：

全谱 `SelfAdjointEigenSolver` 的复杂度为 $O(L^3)$。由于 $r \ll L$（极端情况 $L=512$, $r=2$，$r/L \approx 0.004$），使用 Lanczos 方法仅计算 $r$ 个最大特征对的复杂度为 $O(L^2 r \cdot \text{iter})$。

Lanczos 对于对称矩阵的收敛速度由特征间隔 $\gamma = (\sigma_r - \sigma_{r+1}) / (\sigma_1 - \sigma_r)$ 决定。Ding et al. 的条件 $\mu_{\text{tail}} \leq \mu_r/8$ 保证了特征间隙足够大，Lanczos 可在 $O(\log(1/\epsilon)/\sqrt{\gamma})$ 次迭代内收敛。

**备选方案**：
- Eigen 不内置 Lanczos，可使用 **Spectra 库**（`SymEigsSolver`）或自行实现功率迭代 + 收缩法（幂法 + Gram-Schmidt 正交化）
- 功率迭代：$\tilde{v}_{k+1} = R v_k / \|R v_k\|$，收敛率 $|\sigma_{r+1}/\sigma_r|^k$

---

### Step 5: 旋转不变选择矩阵

#### 5.1 复值选择矩阵

定义两个 $(L-1) \times L$ 矩阵，分别选取 $E_s$ 的"前 $L-1$ 行"和"后 $L-1$ 行"：

$$
J_1 = [I_{L-1} \;|\; 0], \quad J_2 = [0 \;|\; I_{L-1}]
$$

#### 5.2 实值域变换（Haardt 式 (32)）

$$
K_1 = \operatorname{Re}\!\big\{ Q_{L-1}^H \cdot (J_1 + J_2) \cdot Q_L \big\} \in \mathbb{R}^{(L-1) \times L}
$$

$$
K_2 = \operatorname{Re}\!\big\{ Q_{L-1}^H \cdot j(J_1 - J_2) \cdot Q_L \big\} \in \mathbb{R}^{(L-1) \times L}
$$

**优化要点**：
- $K_1, K_2$ **仅依赖于 $L$（即仅依赖于 $N$）**，应预计算一次并缓存
- 对于 ULA 结构，$K_1, K_2$ 具有**稀疏三对角带状结构**（Haardt 论文 §IV-D 示例），显式构造仅需 $O(L)$

---

### Step 6: 最小二乘求解旋转算子

#### 6.1 旋转不变方程

$$
K_1 \cdot E_s \cdot \Upsilon \approx K_2 \cdot E_s
$$

其中 $\Upsilon \in \mathbb{R}^{r \times r}$ 为旋转算子。

#### 6.2 LS 解（正规方程）

$$
\Upsilon = (K_1 E_s)^+ \cdot (K_2 E_s) = \big[(K_1 E_s)^T (K_1 E_s)\big]^{-1} \cdot (K_1 E_s)^T \cdot (K_2 E_s)
$$

$K_1 E_s$ 和 $K_2 E_s$ 均为 $(L-1) \times r$ 矩阵，$r \leq 4$。

**优化要点**：
- **LS 代替 TLS**：Haardt & Nossek (Fig. 10) 证明在全部 SNR 范围内 LS 与 TLS 精度曲线重合（误差差异 $\ll 0.1^\circ$）。TLS 需要一次额外的 $(L-1) \times 2r$ 的 SVD，代价显著高于正规方程的 $r \times r$ 求逆（$r \leq 4$ 时可手工写出解析逆）
- 完整 LS 解（SVD 伪逆）可作为后备方案，但正规方程已足够

---

### Step 7: 特征值分解与频率映射

#### 7.1 实矩阵 EVD

$$
\Upsilon = T \cdot \Omega \cdot T^{-1}, \quad \Omega = \operatorname{diag}(\omega_1, \dots, \omega_r)
$$

其中 $\omega_k \in \mathbb{R}$（Unitary ESPRIT 的**可靠性检验**：若任何 $\omega_k$ 有显著虚部，则估计不可靠，应增大 $N$ 或跳过此调用）。

**优化要点**：
- **使用 `Eigen::EigenSolver<MatrixXd>`（实数求解器），而非 `ComplexEigenSolver`**。数学上 $\Upsilon$ 严格为实矩阵，实数 EVD 计算的复杂度仅为复数的 $1/4$（实数 vs 复数算术）。当前代码此处为可修正的 bug。

#### 7.2 空间频率提取（Haardt 式 (33)）

$$
\mu_k = 2\arctan(\omega_k), \qquad k = 1,\dots,r
$$

$$
\hat{f}_k = \frac{|\mu_k|}{2\pi} \cdot f_s
$$

**物理含义**：
- $r=2$：$\omega_1 = -\omega_2$，$\mu_1 = -\mu_2$，$\hat{f}_1 = \hat{f}_2$ → 单一正频率
- $r=4$：两对 $\pm \omega$，对应两个不同的正频率（真频和干扰）

---

### Step 8: 真频甄别

当 $K=2$（有干扰）时输出两个正频率。通过以下策略选取真频：

$$
\hat{f}_{\text{true}} = \arg\min_{\hat{f}_k > 0} \big| \hat{f}_k - f_{\text{ref}} \big|
$$

其中 $f_{\text{ref}}$ 可通过 $f_s \cdot k_{\max}/N$ 从 FFT 幅度谱粗略估计（复用项目中已有的 `findPeaksFromDft`）。

**备选方案**（无参考频率时）：返回所有正频率，交由 Metric 层按最小误差选择（项目当前的 `PercentageErrorMetric` 已在 `estimate()` 调用后处理选峰逻辑——无需修改 Estimator 接口）。

---

## 5. 完整流水线公式汇总

$$
\boxed{
\begin{aligned}
&\textbf{输入: } x[0{:}N{-}1] \in \mathbb{R}^N \;(N \in [32,1024]),\; f_s,\; K \in \{1,2\} \;(r = 2K) \\
\\
&\textbf{① }\;L \gets \lfloor N/2 \rfloor,\; M \gets N-L+1,\;
X \in \mathbb{R}^{L \times M} \gets \text{Hankel}(x) \\
\\
&\textbf{② }\;Z_{\text{CH}} \gets \big[X \;\; \Pi_L X \Pi_M\big] \in \mathbb{R}^{L \times 2M} \\
\\
&\textbf{③ }\;T_X \gets \operatorname{Re}\{ Q_L^H Z_{\text{CH}} Q_{2M} \} \in \mathbb{R}^{L \times 2M} \quad\text{(Q 矩阵预计算)} \\
\\
&\textbf{④ }\;R \gets T_X T_X^T \in \mathbb{R}^{L \times L},\quad 
E_s \gets \text{Lanczos-top-}r(R) \in \mathbb{R}^{L \times r} \\
\\
&\textbf{⑤ }\;K_1,K_2 \in \mathbb{R}^{(L-1) \times L} \gets \text{预计算常量选择矩阵} \\
\\
&\textbf{⑥ }\;\Upsilon \gets (K_1 E_s)^T(K_1 E_s)^{-1} (K_1 E_s)^T(K_2 E_s) \in \mathbb{R}^{r \times r} \\
\\
&\textbf{⑦ }\;\Upsilon = T\Omega T^{-1},\; \omega_k \in \mathbb{R},\;
\mu_k \gets 2\arctan(\omega_k),\;
\hat{f}_k \gets |\mu_k| f_s / (2\pi) \\
\\
&\textbf{⑧ }\;\textbf{输出: } \{\hat{f}_k > 0\} \rightarrow \hat{f}_{\text{true}} \gets \arg\min |\hat{f}_k - f_{\text{ref}}| \\
\end{aligned}
}
$$

---

## 6. 当前实现优化诊断对照

| 当前代码（`esprit.cpp`） | 优化方案 | 影响 |
|---|---|---|
| `SelfAdjointEigenSolver` 全谱 EVD on $R_T$ | Lanczos top-$r$ 或功率迭代 | $L \gg r$ 时加速 $O(L/r)$ 倍 |
| `T_X * T_X^T / (2*M)` | 去掉 $/(2M)$ | 节省 $O(L^2)$，零精度损失 |
| `ComplexEigenSolver` 求解 $\Upsilon$ | 换用 `EigenSolver<MatrixXd>` | 加速 4×，消除数值虚部 |
| `bdcSvd().solve()` 做伪逆 | 正规方程 $(A^T A)^{-1}A^T$ | $r \leq 4$ 时几乎免费 |
| $Q_L, K_1, K_2$ 每次重建 | 预计算缓存 | 在线调用归零 |
| `Z_CH` 和 $T_X$ 通过复杂乘法 | 可用 Haardt 式 (7) 块公式 | $O(L^3) \to O(L^2)$ 可选优化 |

---

## 7. 精度-复杂度权衡与理论保证

### 7.1 误差上界

Ding et al. 定理 III.1（正式版）给出 ESPRIT 频率估计的最优误差上界：

$$
\mathrm{md}(\hat{z}, z_{\text{dom}}) = \mathcal{O}\!\left(\left(\frac{\alpha\sqrt{\log n}}{\mu_r\sqrt{\Delta_z}} + r^{3/2}\right) \frac{r \alpha^2 \log n}{\mu_r^2 \Delta_z n^{3/2}}\right)
$$

项目参数代入（$\alpha \sim \text{noise std}$，$\mu_r = \min(A_k/2) \approx 0.5$，$\Delta_z = 2\pi\Delta_f/f_s$，$n = N$）：

- $N = 256$，无干扰（$r=2$），SNR = 10 dB：$\epsilon \sim \tilde{\mathcal{O}}(N^{-3/2}) \approx 10^{-3.6} \approx 2.5\times 10^{-4}$ 归一化频率
- 转换为 Hz：$\epsilon \cdot f_s/(2\pi) \approx 0.04$ Hz（$f_s=1000$）

所有优化（Lanczos, LS, 实 EVD）均不改变此误差边界——它们只影响数值计算的精度，而不改变估计算法的统计效率。

### 7.2 何时精度会损失

| 场景 | 影响 | 缓解 |
|---|---|---|
| 两个频率太接近（$\Delta_z$ 很小） | 特征间隙缩小，子空间分离困难 | 增加 $N$（Ding 定理需要 $N = \Omega(1/\Delta_z)$） |
| SNR 极低（$< -10$ dB） | 噪声子空间污染信号子空间，$\Upsilon$ 特征值虚部增大 | 可靠性检验自动捕获；增加 $N$ 或迭代次数 |
| 干扰和真频幅度差异过大（$A_{\text{int}} \ll A_{\text{true}}$） | 干扰对应的特征值可能低于噪声阈值，$r$ 过估计 | 当前 $r$ 由 `FrequencyCount` 外部传入，不依赖阈值 |
| Lanczos 迭代次数不足 | 特征向量未严格收敛 | 设置合理的 stopping criteria（如 $\|R\tilde{v} - \tilde{\sigma}\tilde{v}\| < \epsilon$） |

---

## 8. 参考论文引用细节

### 论文 1: Ding et al. 2024

```
Z. Ding, E. N. Epperly, L. Lin, and R. Zhang,
"The ESPRIT algorithm under high noise: Optimal error
 scaling and noisy super-resolution,"
in Proc. IEEE 65th Annu. Symp. Found. Comput. Sci. (FOCS),
2024, pp. 2344–2361. DOI: 10.1109/FOCS61266.2024.00137
```

- **定位**：本项目已引用的 ESPRIT 理论参考（`esprit.h` 注释中引用）
- **关键贡献**：定理 I.4 / III.1 证明 ESPRIT 达到 $\tilde{\mathcal{O}}(n^{-3/2})$ 最优误差缩放；引理 I.9 / IV.1 给出二阶特征空间摄动展开
- **实验用途**：为算法精度提供理论下界；指导 $N$ 的选择

### 论文 2: Haardt & Nossek 1995

```
M. Haardt and J. A. Nossek,
"Unitary ESPRIT: How to obtain increased estimation
 accuracy with a reduced computational burden,"
IEEE Trans. Signal Process., vol. 43, no. 5, pp. 1232–1242,
May 1995. DOI: 10.1109/78.382406
```

- **定位**：本项目当前 ESPRIT 实现的算法基础
- **关键贡献**：式 (3-4) $Q_n$ 矩阵；式 (7) 实值变换显式公式；式 (32) $K_1,K_2$ 选择矩阵；式 (33) $\mu_k = 2\arctan(\omega_k)$；注记 3 LS 解；命题 2 可靠性检验；Fig. 10 LS ≈ TLS 经验证据

---

## 9. 注意事项与实现提示

### 9.1 编程注意事项

1. **$Q$ 矩阵的偶/奇分支**：创建 `Q_n` 时注意分支 $n$ 的奇偶性（偶数：式 (3)；奇数：式 (4)），本项目 $N$ 通常为偶数，$L = N/2$ 可能为奇数或偶数。
2. **实数 EVD 提取正频率**：`EigenSolver` 的 `eigenvalues()` 返回 `VectorXcd`，但实特征值的 `imag()` 部分应为零。提取 `real()` 部分后做 `2*atan(w)`。注意 `atan` 返回 $\mu_k \in (-\pi, \pi)$。
3. **矩阵维度检查**：$K_1, K_2$ 为 $(L-1) \times L$，$E_s$ 为 $L \times r$，$K_1 E_s$ 为 $(L-1) \times r$。当 $L-1 < r$（$N < 6$）时方程组欠定——本项目 $N \geq 32$，不会出现。
4. **预计算缓存机制**：`EspritEstimator` 的成员变量应缓存 `Q_L`, `Q_{L-1}`, `Q_{2M}`, `K1`, `K2`。由于 `estimate()` 每次调用 $N$ 可不同（`input.size()`），需惰性初始化（以 `N` 为 key 的哈希表）。
5. **Lanczos 的 Eigen 替代**：Eigen 无内置 Lanczos，可使用 **Spectra** (`SymEigsSolver`) 头文件库。若不想引入新依赖，可手写幂法 (power iteration) + Gram-Schmidt 正交化（对于 $r \leq 4$，幂法足够稳定）。

### 9.2 测试注意事项

- **可靠性检验**：建议在调试构建中启用（确保 $\Upsilon$ 特征值虚部 $\ll$ 实部），在发布构建中可作为错误检查
- **回归测试**：当前 `esprit.cpp` 对 N=256、单频正弦的估值应在新实现上得到相同结果（允许机器 epsilon 级别的差异）

### 9.3 与本项目代码的接口兼容性

优化后的计算流程保持 `IEstimator` 接口不变：

```cpp
std::vector<FrequencyPeak>
estimate(const RealArray &input, const EstimationContext &context) override;
```

- `FrequencyCount` 传入 $K$（信号数），$r = 2K$
- 仅在 $L$（`input.size()`）变化时重建缓存
- 返回所有正频率峰值（含干扰），`Amplitude = AMP_UNKNOWN`，`Prominence = PROMINENCE_UNKNOWN`

---

## 附录 A: Haardt 式 (7) 的实信号简化

当输入 $X$ 为实数时，Haardt 式 (7) 可大幅简化。令 $L=2k$（偶数情况），划分 $X = [X_1; X_2]$，$X_1, X_2 \in \mathbb{R}^{k \times M}$：

$$
T_X \gets \begin{bmatrix}
X_1 + \Pi_k X_2 \Pi_M & \mathbf{0} \\
\mathbf{0} & X_1 - \Pi_k X_2 \Pi_M
\end{bmatrix} \in \mathbb{R}^{L \times 2M}
$$

此式完全避开复数运算，直接用 $X$ 的块矩阵构造 $T_X$。但需注意 $\Pi_k X_2 \Pi_M$ 是同时反转行和列，在 Eigen 中可用 `.colwise().reverse().rowwise().reverse()` 实现。

奇数 $L=2k+1$ 情况的公式同理（增加中间行）。此优化编码成本低、数值精度零损失。

---

## 附录 B: 参考测试参数

验证优化后算法在测试套件中使用的参数范围（来自 `ScanTestRunner::buildDefaultTests()`）：

| 参数 | 最小值 | 最大值 | 默认值 |
|---|---|---|---|
| $N$ (SampleCount) | 32 | 1024 | 256 |
| $f_s$ (SampleRateHz) | 7680 (Test 2) | 1000 (默认) | 1000 |
| $f_{\text{true}}$ (FrequencyHz) | 200 | 1530 | 200 |
| SNR | -30 dB | 20 dB | 10 dB |
| $K$ (频率数) | 1 | 2 | 1 |
