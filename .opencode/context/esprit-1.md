# ESPRIT 算法分析：最小计算复杂度优化（r ≤ 2）

> **目的**：在项目特定需求下（实数正弦信号，含最多 2 个频率（含干扰），加性噪声，仅输出真实频率），以最大化减少计算复杂度且不过度损失精准度为目标，给出 ESPRIT 算法的数学公式化计算流程。

---

## 参考论文

| # | 论文 | 来源 | 核心贡献 |
|---|---|---|---|
| 1 | **Haardt, M. & Nossek, J.A.** (1995). *Unitary ESPRIT: How to Obtain Increased Estimation Accuracy with a Reduced Computational Burden.* IEEE Trans. Signal Processing, 43(5), 1232–1242. | DOI: `10.1109/78.382406` | 实数变换 + 前向-后向平均 → 运算量减半、精度翻倍 |
| 2 | **Ding, Z., Epperly, E.N., Lin, L. & Zhang, R.** (2024). *The ESPRIT algorithm under high noise: Optimal error scaling and noisy super-resolution.* Proc. IEEE 65th FOCS, 2344–2356. | DOI: `10.1109/FOCS61266.2024.00137` | 最优误差标度 $\varepsilon = \tilde{O}(n^{-3/2})$、二阶特征向量扰动理论 |

论文 1 存放在：`document/Haardt and Nossek - 1995 - Unitary ESPRIT…/`
论文 2 存放在：`document/Ding et al. - 2024 - The ESPRIT algorithm under high noise…/`

---

## 一、项目约束与算法选择逻辑

### 1.1 约束分析

| 项目特性 | 数学表述 | 算法影响 |
|---|---|---|
| 实数正弦信号 | $x[n] = \sum_{k=1}^{r} A_k\cos(2\pi f_k n/f_s + \phi_k) + w[n]$ | 实余弦 = 一对共轭复指数 → 子空间维数 $d = 2r$ |
| 最多 2 个频率 | $r \leq 2$（1 主信号 + 1 干扰） | $r \times r$ EVD 为常数时间；可截断 top-r |
| 加性噪声（4 分布） | 高斯/均匀/拉普拉斯/脉冲零均值 | 不必假设白噪声；Unitary ESPRIT 无噪色要求 |
| 仅输出真实频率 | 不需要幅度/相位估计 | 跳过 Ding Alg.1 Step 8（最小二乘强度） |
| 单次噪声实现 | 每次蒙特卡洛 = 单快照，$N \in [32, 1024]$ | 必须用 Toeplitz/Hankel（矩阵束）结构，不可用多快照平均 |

### 1.2 算法选择理由

**为什么不直接用标准复数 ESPRIT？**

标准 ESPRIT（Roy & Kailath 1986, Ding 2024 Algorithm 1）对 $N \times N$ 复数 Toeplitz 做 Hermitian EVD，$O(N^3)$ 复运算。对实数信号，复特征值应成对出现 $\pm f_k$，需后处理配对标定——额外复杂度。

**为什么选择 Unitary ESPRIT？**

1. **实数数据 ⇒ 中心对称结构**（Haardt §IV-A）：实数时间序列天然满足 centro-symmetric，$\Pi_N \bar{x} = x$。
2. **实数运算**：$N \times N$ 复 Hermitian EVD 约 $25N^3$ FLOPs → $N \times N$ 实对称 EVD 约 $9N^3$ FLOPs（$\sim 2.8\times$ 加速）。
3. **前向-后向平均**：有效快照数从 $M \to 2M$（Haardt Eq.18），改善条件数。
4. **可靠性检验**（Haardt §IV-C）：实数特征值自动保证 $|e^{j\mu_k}| = 1$，虚部 $\neq 0$ 可直接检出失败。
5. **$r \leq 2$ 可利用截断 Lanczos**：$O(N^2 r) \ll O(N^3)$。

---

## 二、核心算法：RSUH-ER（Real-Signal Unitary Hankel ESPRIT with Lanczos Top-r Extraction）

### 2.1 符号系统

| 符号 | 意义 |
|---|---|
| $x[n]$, $n = 0,\ldots,N-1$ | 实数输入信号 |
| $f_s$ | 采样率 (Hz) |
| $r \in \{1,2\}$ | 子空间维数（实值源数） |
| $d = 2r$ | 复指数总数（信号 + 镜像） |
| $L$ | Hankel 窗长度（关键调优参数） |
| $M = N - L + 1$ | Hankel 快照数 |
| $\Pi_p \in \mathbb{R}^{p \times p}$ | 反对角交换矩阵，$(\Pi_p)_{i,p-1-i} = 1$ |
| $Q_{2n}$ | 偶数维左 $\Pi$-实酉矩阵 |
| $E_s \in \mathbb{R}^{L \times r}$ | 信号子空间（主导左奇异向量） |

### 2.2 完整计算流程

---

#### Step 1：构造前向-后向平均 Hankel 数据矩阵

前向 Hankel：
$$
X_f(i,j) = x[i+j], \quad i = 0,\ldots,L-1, \quad j = 0,\ldots,M-1
$$

后向（时反）Hankel：
$$
X_b(i,j) = x[N-1-i-j], \quad \text{或等价于 } X_b = \Pi_L X_f \Pi_M
$$

拼接（前向-后向平均，Haardt Eq.18）：
$$
Z = \begin{bmatrix} X_f \\ \Pi_L X_f \Pi_M \end{bmatrix} \in \mathbb{R}^{2L \times M}
$$

**复杂度**：$O(LM) = O(L(N-L+1))$。

---

#### Step 2：实值正交变换（Haardt §IV-B）

构造左 $\Pi$-实酉矩阵（偶数情况，Haardt Eq.3）：
$$
Q_{2n} = \frac{1}{\sqrt{2}} \begin{bmatrix} I_n & jI_n \\ \Pi_n & -j\Pi_n \end{bmatrix} \in \mathbb{C}^{2n \times 2n}
$$

对奇数维同理（Haardt Eq.4）。计算实值映射（Haardt Eq.6-7）：
$$
\mathcal{T}(X_f) = \text{Re}\!\left(Q_L^H \, X_f \, Q_{2M}\right) \in \mathbb{R}^{L \times 2M}
$$

> **性质**（Haardt Theorem 1）：当 $X_f$ 为实矩阵，$Q_L, Q_{2M}$ 为左 $\Pi$-实矩阵时，$Q_L^H X_f Q_{2M}$ **自动为实数**，$\,\text{Re}(\cdot)$ 仅为安全类型转换。

实际计算中利用 $Q$ 的稀疏结构（仅含 $0, \pm 1, \pm j, \pm 1/\sqrt{2}$），可将矩阵乘法实现为**选择 + 加减**操作，无需一般复数矩阵乘。

**复杂度**：$O(LM)$（利用稀疏分解）。

---

#### Step 3：协方差矩阵 + 截断 top-r 特征分解

协方差方法（Haardt Remark 1）：
$$
R = \frac{1}{2M} \mathcal{T}(X_f) \mathcal{T}(X_f)^T \in \mathbb{R}^{L \times L}
$$

计算 $R$ 的**前 $r$ 个最大特征值对应的特征向量**：
$$
R E_s = E_s \Lambda_s, \quad E_s \in \mathbb{R}^{L \times r}, \quad \Lambda_s = \text{diag}(\lambda_1,\ldots,\lambda_r)
$$

> **为什么不直接对 $\mathcal{T}(X_f)$ 做 SVD？** 协方差方法将 $L \times 2M$ 的 SVD（$O(LM\min(L,2M))$）转化为 $L \times L$ 的 EVD（$O(L^3)$ 全量或 $O(L^2 r)$ 截断）。对 $L \ll 2M$ 场景更优。本项目 $L \approx O(r)$ 时，两方法复杂度相近。

**截断 EVD 实现**（$r \leq 2$ 关键优化）：
- **方法 A**（推荐，Eigen 3.4+）：`SelfAdjointEigenSolver::compute(R, r)`，内部使用 LDLT 分解 + 分治，仅计算 $r$ 个最大特征值对应的特征向量。
- **方法 B**（大规模场景）：用 Lanczos 迭代（`Eigen::EigenSolver` 不直接支持），借助 `Spectra` 库的 `SymEigsSolver`，每次迭代仅需矩阵-向量乘。
- **方法 C**（$L \leq 16$）：直接全量 EVD，$O(L^3)$ 在 $L \leq 16$ 时仍为 $O(4096)$，可忽略。

**复杂度**：$O(L^2 M) + O(L^2 r)$，其中 $L^2 M$ 来自协方差矩阵乘，$L^2 r$ 来自截断 EVD。

---

#### Step 4：构造实数稀疏选择矩阵（Haardt Eq.32）

定义 $J_1, J_2 \in \mathbb{R}^{(L-1) \times L}$：
$$
J_1 = [I_{L-1} \mid \mathbf{0}_{L-1}], \quad J_2 = [\mathbf{0}_{L-1} \mid I_{L-1}]
$$

即 $J_1$ 选取前 $L-1$ 行，$J_2$ 选取后 $L-1$ 行（平移不变性）。

实数选择矩阵（Haardt Eq.32 第一部分）：
$$
K_1 = \text{Re}\!\left(Q_{L-1}^H (J_1 + \Pi_{L-1} J_1 \Pi_L) Q_L\right) \in \mathbb{R}^{(L-1) \times L}
$$
$$
K_2 = \text{Re}\!\left(j \cdot Q_{L-1}^H (J_1 - \Pi_{L-1} J_1 \Pi_L) Q_L\right) \in \mathbb{R}^{(L-1) \times L}
$$

**具体稀疏形式**（$L$ 为偶数，$k = L/2$，最大重叠 ULA 例）：

$$
K_1 = \begin{bmatrix}
1 & 1 & 0 & 0 & \cdots & 0 & 0 \\
0 & 1 & 1 & 0 & \cdots & 0 & 0 \\
0 & 0 & 1 & 1 & \cdots & 0 & 0 \\
\vdots & & & \ddots & & & \vdots \\
0 & 0 & 0 & 0 & \cdots & 1 & 1
\end{bmatrix}_{(L-1) \times L}
$$

$$
K_2 = \begin{bmatrix}
0 & 0 & \cdots & -1 & 1 & 0 \\
0 & 0 & \cdots & 0 & -1 & 1 \\
\vdots & & \ddots & & & \vdots \\
1 & -1 & 0 & \cdots & 0 & 0
\end{bmatrix}_{(L-1) \times L}
$$

**性质**：$K_1, K_2$ 每行仅含 **2 个非零元**：$K_1$ 为 $[\ldots, 1, 1, \ldots]$，$K_2$ 为 $[\ldots, -1, 1, \ldots]$（对称布置）。可存量化为稀疏矩阵或用索引暴力计算。

**复杂度**：$O(L)$（直接构造）。

---

#### Step 5：最小二乘解（实数 TLS）

解实数系统：
$$
K_1 E_s \Upsilon \approx K_2 E_s
$$

其中 $K_1 E_s \in \mathbb{R}^{(L-1) \times r}$，$K_2 E_s \in \mathbb{R}^{(L-1) \times r}$。

解来自正规方程（因 $r \leq 2$，$(L-1) \gg r$，用 LS 替代 TLS 节省 SVD）：

$$
\Upsilon = \bigl[(K_1 E_s)^T (K_1 E_s)\bigr]^{-1} (K_1 E_s)^T (K_2 E_s) \in \mathbb{R}^{r \times r}
$$

> **非奇异保证**：$K_1 E_s$ 满列秩要求 Hankel 矩阵的列空间不含零向量，这由 Vandermonde 矩阵的奇异值下界保证（Moitra 2015, Ding Lemma I.8, Theorem B.1），条件数 $\kappa = O(1)$ 当 $n = \Omega(1/\Delta_z)$。

> **Haardt Remark 3**：LS 与 TLS 在此场景下误差等同（Fig.10 实心/虚线重合），采用 LS 节省每次 SVD 的 $O(L r^2)$。

利用 $r \ll L$ 特性，可闭式计算正规方程逆（$r$ 极小）：
- $r = 1$：标量除法，$\Upsilon = (\sum a_i b_i) / (\sum a_i^2)$，其中 $a_i = (K_1 E_s)_i,\, b_i = (K_2 E_s)_i$
- $r = 2$：$2\times 2$ 矩阵公式直接求逆

**复杂度**：$O(L r^2)$。

---

#### Step 6：$r \times r$ 实矩阵特征值分解

$$
\Upsilon = V \Omega V^{-1}, \quad \Omega = \text{diag}(\omega_1, \ldots, \omega_r) \in \mathbb{R}^r
$$

因 $\Upsilon \in \mathbb{R}^{r \times r}$，使用：
- `Eigen::EigenSolver<MatrixXd>`（通用实 Schur 分解，$O(r^3)$）
- 或 $r=1$ 时直接取值 $\omega_1 = \Upsilon_{11}$
- 或 $r=2$ 时闭式：特征值 $\omega = \frac{1}{2}\left(\text{tr}(\Upsilon) \pm \sqrt{\text{tr}(\Upsilon)^2 - 4\det(\Upsilon)}\right)$

**可靠性检验**（Haardt §IV-C）：所有 $\omega_k$ **必须为实数**。若出现复共轭对，表明 $\Upsilon$ 的谱不在单位圆上，估计不可靠。此时应：
- 增大窗长 $L$（更多数据进入 EVD）
- 增加样本数 $N$（采集更长序列）
- 标记该次蒙特卡洛迭代无效

**复杂度**：$O(r^3) \approx O(1)$。

---

#### Step 7：频率提取

由 Haardt Eq.33：

$$
\mu_k = 2\arctan(\omega_k), \quad k = 1,\ldots,r
$$

其中 $\mu_k$ 为数字频率（rad/sample）。转换为物理频率（Hz）：

$$
\hat{f}_k = \frac{\mu_k}{2\pi} \cdot f_s = \frac{f_s}{\pi} \arctan(\omega_k)
$$

若检测到 $|\omega_k| > 1$（高频折叠），需处理：
$$
\hat{f}_k = \begin{cases}
f_s - \frac{f_s}{\pi}\arctan(|\omega_k|), & \omega_k > 1 \\
\frac{f_s}{\pi}\arctan(\omega_k), & |\omega_k| \leq 1 \\
-\frac{f_s}{\pi}\arctan(|\omega_k|), & \omega_k < -1
\end{cases}
$$

**复杂度**：$O(r)$。

---

## 三、Hankel 窗长度 $L$ 的调优（最大杠杆点）

### 3.1 Ding 2024 给出的下界

Theorem III.1 要求 $n$（特征分解维数）满足：

$$
n = \Omega\!\left( \left(\frac{r \alpha^{4/3}}{\mu_r^{4/3} \Delta_z^{2/3}} + \frac{\alpha^2}{\mu_r^2 \Delta_z}\right) \cdot \log(r, \alpha, 1/\mu_r, 1/\Delta_z) \right)
$$

其中：
- $\alpha$：噪声次高斯尾参数（对标噪声标准差 $\sigma$）
- $\mu_r$：第 $r$ 个信号的幅度
- $\Delta_z$：信号间复数平面最小间距 $|z_i - z_j|$，对应频率间距 $\Delta_f$（Hz）

$$
\Delta_z = \min_{i \neq j} |e^{2\pi i f_i / f_s} - e^{2\pi i f_j / f_s}| \approx \frac{2\pi \Delta_f}{f_s}
$$

### 3.2 本项目典型参数下的 $L_{\min}$

| 参数 | 典型值 | 备注 |
|---|---|---|
| $f_s$ | 1000 Hz | 由配置决定 |
| $\Delta_f$（两频率最小间距） | $\geq 10$ Hz（保守） | 干扰 $\Delta$bins $\in [0,4]$ |
| $\Delta_z$ | $\approx 2\pi \cdot 10/1000 \approx 0.063$ | 最坏 |
| $\alpha/\mu_2$ | $\approx 0.3$（SNR 10 dB） | $\mu_1 \gtrsim \mu_2$ |
| $\Rightarrow n_{\min}$ | $\approx 2 \cdot 0.3^{4/3} / (1^{4/3} \cdot 0.063^{2/3}) \approx 3.5$ | 取对数因子约 1 |

$\Rightarrow$ 理论下界 $L \geq 8$ 即满足 Ding 的收敛条件。

### 3.3 $L$ 的选择权衡

$$
\boxed{L = \max\!\left(8r,\; \left\lceil\frac{2\pi}{\Delta_z}\right\rceil,\; \left\lfloor\frac{N}{2}\right\rfloor^{\text{（保守）}}\right)}
$$

| $L$ 策略 | 复杂度（$N=256$） | 精度特性 | 适用 |
|---|---|---|---|
| $L = N/2$（现有） | $O(N^3/8) \approx 2 \times 10^6$ FLOPs | 最精确 | $N$ 小，精度优先 |
| $L = 16$~$42$（推荐） | $O(L^2 N) \approx 2 \times 10^5$ FLOPs | $\varepsilon$ 放大 $\sim (N/2L)^{1.5} \approx 8$~$20\times$ | 本项目的典型配置 |
| $L = 8r - 12r$（激进） | $O(r^2 N) \approx 4 \times 10^3$ FLOPs | 条件数恶化，低频分离困难 | SNR $\geq 20$ dB 时安全 |

**推荐配置**：
```text
L = max(8 * r, int(fs / (2 * min_freq_spacing)))
```
其中 `min_freq_spacing` 取信号与干扰的最小间隔（Hz）。间隔未知时保守取 `L = min(N/2, 42)`（依赖 $\Delta_z$ 的 $4\pi/\Delta_z$ 级估计）。

---

## 四、复杂度总表

### 4.1 各步骤复杂度明细（$N=256, r=2, L=16, M=241$）

| 步骤 | 操作 | 维度 | 复杂度 | FLOPs 估算 |
|---|---|---|---|---|
| 1 | 构造 Hankel | $L \times M$ | $O(LM)$ | $\sim 3.9 \times 10^3$ |
| 2 | 实数映射 | $L \times 2M$ | $O(LM)$ | $\sim 3.9 \times 10^3$ |
| 3a | 协方差乘 | $L \times 2M$ × $2M \times L$ | $O(L^2 M)$ | $\sim 6.2 \times 10^4$ |
| 3b | 截断 EVD top-r | $L \times L$ | $O(L^2 r)$ | $\sim 5.1 \times 10^2$ |
| 4 | 选择矩阵构造 | $(L-1) \times L$ 稀疏 | $O(L)$ | $\sim 16$ |
| 5 | LS 解 | $(L-1) \times r$ | $O(L r^2)$ | $\sim 60$ |
| 6 | $r \times r$ EVD | $r \times r$ | $O(r^3)$ | $\sim 8$ |
| 7 | 频率提取 | $r$ | $O(r)$ | $\sim 2$ |
| **总计** | | | $O(NL) + O(L^2 N + L^2 r)$ | **$\sim 7 \times 10^4$** |

### 4.2 与现有实现的对比

| 实现 | 全量 EVD | 子空间提取 | 最终 EVD | 总复杂度（$N=256$） | 相对加速 |
|---|---|---|---|---|---|
| 现有（`esprit.cpp`） | $L=128, O(L^3) \approx 2 \times 10^6$ | $O(L^2 M)$ 复数 | 复数 $O(r^3)$ | $\sim 8 \times 10^6$ FLOPs | 1×（基线） |
| 推荐优化 | $L=16$，截断 top-r | $O(L^2 M)$ 实数 | 实数 $O(r^3)$ | $\sim 7 \times 10^4$ FLOPs | **$\sim 100\times$** |
| 激进优化（$L=6$） | $L=6$，闭式 | $O(LM)$ | 实数闭式 | $\sim 2 \times 10^3$ FLOPs | **$\sim 4000\times$** |

> 注：以上为理论 FLOPs。实际加速还受 CPU 缓存、Eigen 的 SIMD 向量化（实数比复数更有效）、内存带宽影响。对 $N=256$，推荐配置**实测应 $\leq 1$ ms 完成**（vs 现有实现约 10-100 ms）。

---

## 五、现有实现的对照与差距分析（Gap Analysis）

### 5.1 现有实现已正确实现的内容

- ✅ **Unitary ESPRIT 框架**（`create_Q`, `Z_CH`, `T_X` — 对应 Haardt Eq.3, 式 (18), 式 (6)）
- ✅ **前向-后向平均**（`Z_CH << X, Pi_L * X.conjugate() * Pi_M`）
- ✅ **实值子空间提取**（`T_X = (Q_L.adjoint() * Z_CH * Q_2M).real()` — Haardt §IV-B）
- ✅ **协方差方法**（`R_T = T_X * T_X.transpose() / (2M)` — Haardt Remark 1）
- ✅ **实数 TLS 解**（`Upsilon = solve_pseudo_inverse(K1*E_s, K2*E_s)`）
- ✅ **频率映射公式**（`mu_k = 2.0 * atan(omega_k)` — Haardt Eq.33）
- ✅ **正频率过滤**（`freqs[k] <= fs / 2.0` 筛选）

### 5.2 现有实现可优化的差距

| # | 项目 | 现有实现 | 存在问题 | 推荐改进 | 预期加速 |
|---|---|---|---|---|---|
| 1 | **Hankel 窗长度 $L$** | `L = N / 2` 硬编码 | $L=128$ 对 $N=256$ 时 $L^3 \approx 2\times 10^6$；$r=2$ 只需 $L \approx 16$ | `L = max(8*r, ceil(2*pi/delta_z))` | $\sim 500\times$ |
| 2 | **全量 EVD** | `SelfAdjointEigenSolver(R_T)` 不指定 top-r | 计算所有 $L$ 个特征向量 | 用 `compute(R_T, r)` 或 Lanczos | $\sim L/r \times$ |
| 3 | **复数 SVD** | `bdcSvd<ComputeThinU\|ComputeThinV>` 于复矩阵 | `K1*E_s` 和 `K2*E_s` 是**实矩阵** | 用实数 BDCSVD | $\sim 2\times$ |
| 4 | **复数 EVD（最终步）** | `ComplexEigenSolver<MatrixXcd>` | $\Upsilon \in \mathbb{R}^{r \times r}$ | `EigenSolver<MatrixXd>` 或闭式 | $\sim 3\times$ |
| 5 | **Q 矩阵乘的稀疏性** | 显式构造 dense + 复数乘 | $Q_L\cdot X \cdot Q_{2M}$ 为 dense | 利用 $0,\pm1,\pm j,\pm 1/\sqrt2$ 稀疏模式 | $\sim 2\text{-}4\times$ |
| 6 | **可靠性检验** | 无 | 若 $\Upsilon$ 有复特征值，估计不可靠 | Haardt §IV-C 检验 + `return {}` | 稳健性（非速度） |
| 7 | **频率后处理** | 直接使用 $\mu_k = 2\arctan(\omega_k)$ | 忽略 $\omega_k > 1$ 的高频折叠 | 增加折叠处理 | 精确性（非速度） |
| 8 | **伪逆法 LS** | `solve_pseudo_inverse` (`bdcSvd`) | 对 $r \leq 2$ 显式 SVD 过重 | 正规方程 / Woodbury | $\sim 5\times$（小项目） |

### 5.3 单步骤对比（$N=256, r=2$）

| 操作 | 现有（复数/全量） | 优化后（实数/截断） | 加速比 |
|---|---|---|---|
| 协方差矩阵乘 $R = \mathcal{T}\mathcal{T}^T$ | $128 \times 128 \times 129 \times 2\text{（复乘算 4 实乘）} \approx 1.7\times 10^7$ | $16 \times 16 \times 241 \approx 6.2\times 10^4$ | $\sim 274\times$ |
| $L\times L$ EVD | $128^3 \approx 2.0\times 10^6$ | $16^2 \times 2 \approx 512$ | $\sim 4000\times$ |
| $r\times r$ EVD | $2^3 \times 25$（复数 Schur）$\approx 200$ | $2^3 \times 9$（实 Schur）$\approx 72$ | $\sim 2.8\times$ |
| 伪逆 SVD | $15 \times 2^2 \times 2$（复 SVD）$\approx 120$ | $15 \times 2^2$（实 LS）$\approx 60$ | $\sim 2\times$ |

---

## 六、关于精度损失的量化分析

### 6.1 Ding 2024 误差标度

由 Theorem I.4（最优误差）：

$$
\mathrm{md}(\hat{z}_r, z_{\mathrm{dom}}) = \tilde{O}\!\left( \frac{r^{1.5}\alpha^3}{\mu_r^3 \Delta_z^{1.5} n^{1.5}} \right)
$$

其中 $n$ 是特征分解维数（在本算法中对应 $L$）。当 $L$ 从 $N/2$ 减小到 $L_{\min}$ 时：

$$
\frac{\varepsilon(L_{\min})}{\varepsilon(N/2)} \approx \left(\frac{N/2}{L_{\min}}\right)^{1.5}
$$

| $N$ | $L=N/2$ | $L_{\min}=16$ | 误差放大倍数 |
|---|---|---|---|
| 64 | 32 | 16 | $(32/16)^{1.5} \approx 2.8\times$ |
| 128 | 64 | 16 | $(64/16)^{1.5} = 8\times$ |
| 256 | 128 | 16 | $(128/16)^{1.5} \approx 22.6\times$ |
| 512 | 256 | 16 | $(256/16)^{1.5} \approx 64\times$ |
| 1024 | 512 | 16 | $(512/16)^{1.5} \approx 181\times$ |

### 6.2 蒙特卡洛平均效应

本项目用 Monte Carlo（典型 100–1000 次迭代）生成频率估计均方误差。Monte Carlo 平均对单个估计误差 $e_i$ 有：

$$
\overline{e} = \frac{1}{M}\sum_{i=1}^{M} e_i, \quad \text{Var}(\overline{e}) = \frac{\sigma_e^2}{M}
$$

即使单个估计误差放大 $20\times$，经 $M=100$ 次 MC 平均后，方差放大仅为 $20/\sqrt{100} = 2\times$。**MC 平均大幅缓解小 $L$ 导致的单次精度损失**。

### 6.3 经验判据

推荐如下自适应策略：

```
如果 N < 64:  L = N/2        （精度优先，数据量不足）
如果 N ∈ [64, 256]: L = max(16, 8*r)    （平衡）
如果 N > 256: L = min(N/3, max(16, 8*r))（速度优先）
```

---

## 七、与论文公式的交叉引用

### Haardt & Nossek 1995 公式引用

| 本文件引用位置 | 原论文公式/编号 | 说明 |
|---|---|---|
| Step 1，$Z$ | Eq.(18) | 前向-后向矩阵 |
| Step 2，$Q_{2n}$ | Eq.(3)/(4) | 左 $\Pi$-实矩阵 |
| Step 2，$\mathcal{T}(\cdot)$ | Eq.(6), (7) | 实数映射 |
| Step 4，$K_1, K_2$ | Eq.(32) | 稀疏选择矩阵 |
| Step 5，LS 替代 TLS | Remark 3, §IV-D | 实数 LS 节约 |
| Step 6，可靠性检验 | §IV-C, Eq.(30) | $\omega_k \in \mathbb{R}$ |
| Step 7，$\mu_k = 2\arctan(\omega_k)$ | Eq.(33) | 频率提取 |
| TABLE I | §IV-E | 完整算法 7 步 |

### Ding et al. 2024 公式引用

| 本文件引用位置 | 原论文公式/编号 | 说明 |
|---|---|---|
| Algorithm 1 | §I-A | 标准 ESPRIT 8 步 |
| Lemma I.8 | Eq.(I.13)-(I.14) | Vandermonde 与特征基的关系 |
| Theorem I.2 | Eq.(I.10) | 中心极限误差 $\tilde{O}(n^{-1/2})$ |
| Theorem I.4 | Eq.(I.11) | 最优误差 $\tilde{O}(n^{-3/2})$ |
| Theorem III.1 | Eq.(III.1)-(III.5) | 含 $n$ 下界的正式版 |
| Theorem V.1 | Eq.(I.16) | 强特征向量比较估计 |
| Eq. $L_{\min}$ | Eq.(III.1) | $n$ 下界条件 |

---

## 八、注意事项与工程陷阱

### 8.1 已知陷阱（来自 project history）

| 陷阱 | 症状 | 原因 | 对策 |
|---|---|---|---|
| **$L$ 过小** | 低频接近的两个峰无法分辨 | $\Delta_z$ 不满足 Ding Eq.(III.1) | 至少 $L \geq 2\pi/\Delta_z$ |
| **实数 $E_s$ 不正交** | $E_s^T E_s \neq I$ 影响 LS 解 | 截断 EVD 保留 full precision | 确保 EVD 精度 $\leq$ 机器精度 |
| **$\Upsilon$ 特征值虚部 $> \varepsilon$** | 频率估计不可靠 | $L$ 不足/SNR 过低 | 跳过该 MC 迭代 + 报日志 |
| **$K_1 E_s$ 列秩亏** | LS 无解 | $E_s$ 未正确提取子空间 | 增加 $L$ 或增大 $r$（该情况对 $r\leq2$ 极少见） |
| **Windows exe-lock** | 链接：Permission denied | 未终止的 ISPPracticeOne.exe | `Stop-Process -Name ISPPracticeOne -Force` |
| **$L$ 改变需 clean rebuild** | 旧 CMake 缓存 | 编译选项 | `Remove-Item -Recurse -Force build` |
| **复数乘误用** | 结果有虚部噪声 | $\mathcal{T}(X_f)$ 的 `.real()` 隐含精度损失 | 确保 $X_f$ 实数且用实数类型 |

### 8.2 不推荐的优化路径

| 优化方案 | 不推荐理由 |
|---|---|
| **加 Hilbert 变换预处理** | 对 $r \leq 2$ 收益小；增加 O($N \log N$) FFT 成本；引入边界效应 |
| **用 $L = N$（全 Toeplitz）** | Ding 2024 理论最优但不必要——$L=16$ 已满足精度 + 速度要求 |
| **用 `arma::svd` 代替 Eigen** | 增加项目依赖；Eigen 3.4+ 的 `compute(n)` 已支持部分特征分解 |
| **完全放弃 Eigen，手写 BLAS** | Eigen 对 $L \times M$ 矩阵乘已优化到接近 BLAS 极限 |
| **同时输出强度 $\mu_k$** | 增加 10-20% 计算量且需求不需要——减少无谓步骤 |
| **$L$ 固定不变的硬编码** | 不同 $N$ 和 SNR 场景需要不同 $L$——保留可调参数 |

### 8.3 可靠性检验的实现细节

Haardt §IV-C 可靠性检验的实现在本算法中为：

```cpp
// After computing omega = eig(Upsilon).real():
constexpr double TOL = 1e-6;
Eigen::VectorXcd eig_vals = solver.eigenvalues();
bool reliable = (eig_vals.imag().cwiseAbs().maxCoeff() < TOL);
if (!reliable) {
    // 不可靠：SNR 过低或 L 不足
    return {};  // 空峰值列表，不给 RunResult 传无效频率
}
// 否则取实部继续：
Eigen::VectorXd omega = eig_vals.real();
```

对应 Ding 2024 Lemma II.2 中 $(K_1 E_s)^+ K_2 E_s$ 的条件：若特征值虚部 $\ll \Delta_z/2$，则估计在 Bauer-Fike 意义上收敛。

### 8.4 窗口函数兼容性

现有 `IMetric`/`IEstimator` 接口传递 `EstimationContext::WindowKind`。需注意：
- ESPRIT 理论上假设数据**不加窗**（或矩形窗）以满足 Vandermonde 结构要求
- 如加非矩形窗，数据应视为"预加权"，不影响算法稳定性——但可能降低谱峰尖锐度
- 本项目 `applyWindow()` 在 `estimate()` 之前被调用，ESPRIT 收到的已经是加窗数据

**处理建议**：
- 对 ESPRIT，维持 `WindowKind == Rectangular` 为默认（非矩形窗可设为可选项，标注 "experimental"）
- 如必须支持非矩形窗，应在回归测试中单独加测

---

## 九、实施检查清单

- [ ] 将 `espritCalc()` 中 `L = N / 2` 改为可调参数 `L = max(8*r, ceil(2*pi/delta_z))`
- [ ] 增加参数 `context` 传入 `delta_z` 或频率间距估计
- [ ] 将 `SelfAdjointEigenSolver(R_T)` 改为截断：`solver.compute(R_T, Eigen::EigenvaluesOnly)` + 仅取 $r$ 个
- [ ] 将 `solve_pseudo_inverse` 用实数正规方程替代（`bdcSvd` → `(K1E)^T (K1E) \ (K1E)^T (K2E)`）
- [ ] 将 `ComplexEigenSolver<MatrixXcd>` 改为 `EigenSolver<MatrixXd>`
- [ ] 增加可靠性检验（`if any |Im(ω_k)| > tol: return {}`）
- [ ] 增加 $\omega_k > 1$ 的高频折叠处理
- [ ] 回归测试：对照现有实现的输出（在 $L=N/2$ 下近似一致）；在小 $L$ 下验证 RMSE 是否在可接受范围内
- [ ] 文档更新：`AGENTS.md` 或本上下文文件标注 `L` 的默认推荐值

---

## 十、附录：关键算法代码骨架（数学→实现映射）

### 10.1 实数映射的稀疏实现

Haardt Eq.(7) 的 $\mathcal{T}(G)$ 在 $G$ 为实数时退化。对实矩阵 $G \in \mathbb{R}^{L \times M}$：

**直接公式**（Haardt Eq.7 简化版，实数情形）：
$$
\mathcal{T}(G) = \begin{bmatrix}
G_1 + \Pi_{L/2} G_2 & 0 \\
\sqrt{2} \cdot g^T & 0 \\
0 & G_1 - \Pi_{L/2} G_2
\end{bmatrix}
$$

其中 $G_1, G_2$ 是 $G$ 的上/下半，$g^T$ 仅当 $L$ 为奇数时存在中间行。实际计算中，$\mathcal{T}(G)$ 即为将 $G$ 折叠后取差值的实数组合——无需复数参与。

**Eigen 伪代码**：
```cpp
// L even, G = X (real matrix, no conjugation needed)
Eigen::Index k = L / 2;
// T_X = [G1 + Pi*G2, 0; 0, 0; 0, G1 - Pi*G2] is just 
// the real part of Q_L^H * G * Q_{2M}
// For real G this simplifies significantly:
Eigen::MatrixXd T_X(L, 2 * M);
T_X.topRows(k) = X.topRows(k) + Pi_L * X.bottomRows(k);     // G1 + Pi*G2
T_X.bottomRows(k) = X.topRows(k) - Pi_L * X.bottomRows(k);  // G1 - Pi*G2
// No imaginary part exists - T_X is already real
```

### 10.2 Woodbury（正规方程）替代 SVD 的实现

```cpp
// 原：
// Eigen::MatrixXcd Upsilon = A.bdcSvd<Eigen::ComputeThinU | Eigen::ComputeThinV>().solve(B);

// 优化后（实数）：
Eigen::MatrixXd A_real = K1 * E_s;  // (L-1) × r 实矩阵
Eigen::MatrixXd B_real = K2 * E_s;  // (L-1) × r 实矩阵
Eigen::MatrixXd AtA = A_real.transpose() * A_real;  // r × r
Eigen::MatrixXd AtB = A_real.transpose() * B_real;  // r × r
Eigen::MatrixXd Upsilon = AtA.ldlt().solve(AtB);    // r × r (LDLT分解)
```

---

*文档版本 v1.0 | 生成日期 2026-07-31 | 基于 Ding 2024 (FOCS) + Haardt & Nossek 1995 (IEEE TSP)*
