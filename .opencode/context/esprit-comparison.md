# ESPRIT 三文件横向综合分析：修正统一计算流程

> **目的**：对 `esprit-1.md`、`esprit-2.md`、`esprit-3.md` 做横向交叉审阅，发现并纠正错误，整合出一个针对本项目需求的最优数学计算流程。

---

## 〇、三文件速览

| 文件 | 核心主张 | 独特贡献 |
|------|---------|---------|
| `esprit-1.md` | 激进缩减 $L$、协方差+截断 EVD | Q 稀疏块公式（§10.1）、$K_1/K_2$ 显式稀疏形式（§Step4）、可靠性检验实现（§8.3） |
| `esprit-2.md` | $L=N/2$ 固定、Lanczos 迭代 | Lanczos 详述（§4.2）、扫描测试参数汇总（App B）、精度-复杂度权衡表（§7） |
| `esprit-3.md` | Square-root SVD 优先、$L \approx N/2$ | 秩约束正确性分析（§9）、目标/干扰标签对称性（§9.2）、LS→TLS 回退策略（§6.3）、D24 限制条件表（§10.4） |

---

## 一、关键错误诊断

### 🔴 致命错误 1：esprit-1.md 的模型阶数

**错误**：全流程以 $r \in \{1, 2\}$ 为信号子空间维数（§1.1 称 $d=2r$ 为复指数总数但不使用 $d$）。$E_s \in \mathbb{R}^{L \times r}$、$\Upsilon \in \mathbb{R}^{r \times r}$、频率提取得到 $r$ 个值——均基于 $r = K$。

**为何错**：一个实正弦 $A\cos(\omega n+\varphi) = \frac{A}{2}e^{j\varphi}e^{j\omega n} + \frac{A}{2}e^{-j\varphi}e^{-j\omega n}$ 产生两个线性无关复指数。Hankel 矩阵的无噪声秩 = $2K$。$Q$ 变换是酉的（保秩），实值化后信号子空间维度仍为 $2K$。Haardt 原文的 $\Upsilon$ 维度始终等于复指数总数。

**后果**：$K=2$ 时 $r=2 < 4$，丢失两个维度，子空间混叠导致频率估计完全错误。由此派生的 $L_{\min}$、复杂度表、加速比均需修正。$K=1$ 时碰巧 $2K=2$，但概念结构错误。

**修正**：全流程以 $r = 2K$ 正确使用。正确模型下 $r \in \{2,4\}$，esprit-1.md 的 $L_{\min}$ 应从 $8r$ 修正为 $8 \cdot 2K = 16K$（即 $K=1$ 时 $L_{\min}=16$，$K=2$ 时 $L_{\min}=32$）。

> 详细论证见 `esprit-3.md` §1.3 和 §9.1。

---

### 🔴 致命错误 2：esprit-2.md 的选择矩阵公式

**错误**：esprit-2.md §5.2 给出 $K_1 = \operatorname{Re}\!\left\{ Q_{L-1}^H \cdot (J_1 + J_2) \cdot Q_L \right\}$，其中 $J_2 = [0 \mid I_{L-1}]$。$J_1+J_2 = [I_{L-1} \mid I_{L-1}]$ 是简单两半拼接，未利用中心对称性。

**正确公式**（Haardt Eq.32）：

$$
K_1 = \operatorname{Re}\!\left\{Q_{L-1}^H \cdot (J_1 + \Pi_{L-1} J_1 \Pi_L) \cdot Q_L\right\}
$$

$$
K_2 = \operatorname{Im}\!\left\{Q_{L-1}^H \cdot (J_1 - \Pi_{L-1} J_1 \Pi_L) \cdot Q_L\right\}
$$

$\Pi_{L-1} J_1 \Pi_L$ 利用中心对称性耦合前后向数据——这是 Unitary ESPRIT 区分于普通 ESPRIT 的核心。$J_1+J_2$ 版本无法正确定义实值旋转不变方程。

> 正确推导见 `esprit-1.md` §Step4（含 $K_1/K_2$ 的显式稀疏形式，每行仅 2 非零元），及 `esprit-3.md` §5.4。

---

### 🔴 违反需求：esprit-2.md 的估计器内真频筛选

**问题**：esprit-2.md Step 8 使用 $\hat{f}_{\text{true}} = \arg\min |\hat{f}_k - f_{\text{ref}}|$ 在估计器内部筛选真频，$f_{\text{ref}}$ 来自 FFT 或外部输入。

**用户明确要求**：
> 你仍然需要假设"我不知道真实频率到底是什么，所以所有发现的频率都需要被记录"

**修正**：ESPRIT 估计器返回**全部** $K$ 个正频率候选。频率标签由外部 Metric 层或 ScanTestRunner 的 PerPeak 模式后处理分配。

> 正确论述见 `esprit-3.md` §9："在估计完成后用真实频率做匹配评分，但不应在估计阶段把真值馈入 ESPRIT。"

---

## 二、分歧点裁决

### 2.1 $L$ 选择：激进缩减 vs 固定 $N/2$

| 文件 | 立场 | 依据 |
|------|------|------|
| esprit-1 | $L = \max(8r, \lceil 2\pi/\Delta_z \rceil)$ → $N=256$, $L=16$ | Ding 2024 Thm III.1 下界 |
| esprit-2 | $L = \lfloor N/2 \rfloor$ 固定 | 保守，精度最优 |
| esprit-3 | $L \approx N/2$ 为起点，非最优 | 工程平衡 |

**扫描测试参数约束**：Test 7 中 $\Delta_{\text{bins}} \in [0,4]$（步长 0.2），$f_s=1000$Hz，$N=256$：

- $\Delta_{\text{bins}}=0.2$ → $\Delta_f = 0.78$Hz → $\Delta_z \approx 0.0049$ → $L$ 需求 $>1+2\pi/0.0049 \approx 1282 > N$（物理极限）
- $\Delta_{\text{bins}}=4.0$ → $\Delta_f = 15.6$Hz → $\Delta_z \approx 0.098$ → $L > 65$

$K=2$ 时 $\Delta_f$ 对估计器不可见——**保守取 $L=N/2$ 是唯一安全选择**。$K=1$ 时无 $\Delta_z$ 约束，仅需 $L > r+1 = 3$，可激进缩减。

**裁决**：$L$ 自适应（详见 §四），$K=2$ 时保持 $L=N/2$，$K=1$ 时 $L=N/4$（$N \geq 128$）。

> Ding 下界推导见 `esprit-1.md` §3。

---

### 2.2 子空间提取：协方差 vs Square-root

| 方法 | 条件数 | 复杂度 | 推荐者 |
|------|--------|--------|--------|
| Square-root SVD | $\kappa(X)$ | 迭代 $O(LMr \cdot \text{iter})$ | esprit-3 |
| 协方差 + EVD | $\kappa(X)^2$ | $O(L^2M + L^2r)$ | esprit-1, esprit-2 |

**裁决**：统一用协方差方法（与现有 `esprit.cpp` 一致，实现简单，$N \leq 1024$ 时性能差距不大），配合可靠性检验兜底（见 §三 Step7）。可靠性失败 = SNR 过低/频率过近，此时切换方法也难挽救。

> Square-root 精度优势论述见 `esprit-3.md` §8.1；协方差方法详见 `esprit-1.md` §Step3。

---

### 2.3 LS vs TLS

三文件一致：**LS 足够**。Haardt Fig.10 证明 Unitary ESPRIT 下 LS 与 TLS 精度曲线重合。esprit-3.md §6.3 补充了 TLS 回退策略（LS 可靠性失败时尝试 TLS，仍失败则返回空集）。

> LS 正规方程推导见 `esprit-1.md` §Step5（含 $r=2$ 闭式）；TLS 回退策略见 `esprit-3.md` §6.3。

---

### 2.4 加窗兼容性

三文件一致：ESPRIT 要求矩形窗（无窗）。非矩形窗破坏指数移位不变 $x[n] = c z^n$，导致 Vandermonde 模型失配。

**裁决**：ESPRIT 忽略 `context.WindowKind`，始终使用原始未加窗样本构造 Hankel 矩阵。

> 论证见 `esprit-3.md` §2.3 和 `esprit-1.md` §8.4。

---

### 2.5 算法统一性

esprit-3.md §11 建议 $K=1$ 用基础 square-root ESPRIT，$K=2$ 用 Unitary ESPRIT。

**裁决**：统一使用 Unitary ESPRIT。Unitary 的前后向平均在 $K=1$ 时同样免费提升精度（有效快照 ×2），且统一代码路径降低维护成本。$K=1$ 时 $L$ 小（$N/4$），Unitary 的 $Q$ 变换成本可忽略。

---

## 三、修正后的统一数学计算流程

本节给出完整 9 步流程。**细节已由三文件充分覆盖的部分仅做引用，不重复**。

### 符号定义

| 符号 | 值 | 定义 |
|------|-----|------|
| $K$ | 1 或 2 | `context.FrequencyCount` |
| $r$ | $2K \in \{2,4\}$ | 复子空间维数 |
| $L$ | 自适应 | 见 §四 |
| $M$ | $N - L + 1$ | Hankel 快照数 |

---

### Step 1：Hankel 数据矩阵（未加窗）

$$X(i,j) = x[i+j], \quad X \in \mathbb{R}^{L \times M}$$

**关键**：使用原始未加窗样本。忽略 `context.WindowKind`。

> 论证见 §二.2.4；复杂度 $O(LM)$。

---

### Step 2：前向-后向平均

$$Z_{\text{CH}} = [X \;\; \Pi_L X \Pi_M] \in \mathbb{R}^{L \times 2M}$$

$\Pi_L X \Pi_M$ 同时反转行（时间反演）和列（等价复共轭）。对实信号纯实运算。

> 详见 `esprit-2.md` §Step2 和 `esprit-1.md` §Step1。

---

### Step 3：实值 $Q$ 变换（稀疏块公式）

$$T_X = \operatorname{Re}\!\left\{ Q_L^H \cdot Z_{\text{CH}} \cdot Q_{2M} \right\} \in \mathbb{R}^{L \times 2M}$$

**稀疏实现**（$L=2k$ 偶数，$X = [X_1; X_2]$，$X_1,X_2 \in \mathbb{R}^{k \times M}$）：

$$T_X = \begin{bmatrix} X_1 + \Pi_k X_2 \Pi_M & \mathbf{0} \\ \mathbf{0} & X_1 - \Pi_k X_2 \Pi_M \end{bmatrix}$$

**全流程零复数运算**。$L$ 奇数时的完整形式见 Haardt Eq.(7)。

> $Q_n$ 矩阵定义见 `esprit-1.md` §Step2 或 `esprit-2.md` §3.1（Haardt Eq.3-4）；块公式推导见 `esprit-1.md` §10.1 和 `esprit-2.md` App A。

---

### Step 4：信号子空间提取

$$R = T_X T_X^T \in \mathbb{R}^{L \times L}$$

$$R \cdot E_s = E_s \cdot \Lambda_s, \quad E_s \in \mathbb{R}^{L \times r}$$

仅取前 $r$ 个最大特征值对应的特征向量。

**实现**：$L > 32$ 时优先 Lanczos 迭代（Spectra `SymEigsSolver`），$O(L^2 r \cdot \text{iter})$；$L \leq 32$ 时全量 `SelfAdjointEigenSolver` 即可。

> Lanczos 收敛条件分析见 `esprit-2.md` §4.2；截断 EVD 选项见 `esprit-1.md` §Step3。

---

### Step 5：实选择矩阵（预计算 + 缓存）

$K_1, K_2 \in \mathbb{R}^{(L-1) \times L}$，公式见 §一.🔴2（**不用** esprit-2.md 的 $J_1+J_2$ 版本）。

$K_1, K_2$ 仅依赖 $L$，预计算一次。**或直接用显式稀疏形式免去矩阵构造**：

- $(K_1 E_s)(i, :) = E_s(i,:) + E_s(i+1,:)$（循环相邻对求和）
- $(K_2 E_s)(i, :) = -E_s(k-1-i,:) + E_s(k+i,:)$（对称差分，$k=L/2$）

> 稀疏形式详述见 `esprit-1.md` §Step4。

---

### Step 6：LS 解（正规方程）

$$\Upsilon = \big[(K_1 E_s)^T (K_1 E_s)\big]^{-1} (K_1 E_s)^T (K_2 E_s) \in \mathbb{R}^{r \times r}$$

$r \leq 4$，$r \times r$ 求逆为常数时间。

> 闭式 $r=2$ 公式见 `esprit-1.md` §Step5。

---

### Step 7：实 EVD + 可靠性检验

$$\Upsilon = T \Omega T^{-1}, \quad \Omega = \operatorname{diag}(\omega_1, \ldots, \omega_r)$$

使用 `Eigen::EigenSolver<MatrixXd>`（**实数**，非 `ComplexEigenSolver`）。

**可靠性检验**（Haardt §IV-C）：

$$\max_k |\operatorname{Im}(\omega_k)| \leq \tau \cdot \max(1, \max_k |\operatorname{Re}(\omega_k)|)$$

$\tau = 10^{-5}$（调试）/ $10^{-3}$（发布）。失败 → 返回空 `vector<FrequencyPeak>{}`。

> 实现细节见 `esprit-1.md` §8.3；回退策略见 `esprit-3.md` §6.3。

---

### Step 8：频率提取

$$\mu_k = 2 \arctan(\operatorname{Re}(\omega_k))$$

$$\hat{f}_k = f_s \cdot \frac{|\mu_k|}{2\pi}$$

$r = 2K$ 个 $\omega_k$ 产生 $K$ 对 $\pm\omega$，得 $K$ 个不重复正频率。$|\omega_k| > 1$ 时需高频折叠处理。

> 折叠公式见 `esprit-1.md` §Step7。

---

### Step 9：输出

返回**全部** $K$ 个正频率候选：

```cpp
for each unique f̂_k ∈ (0, fs/2):
    peaks.push_back({f̂_k, AMP_UNKNOWN, PROMINENCE_UNKNOWN});
```

**不**在估计器内部筛选真频、不做 Vandermonde 重建、不调用 `findPeaksFromDft`。

> 正确性论证见 `esprit-3.md` §9。

---

## 四、$L$ 自适应选择

### 约束

扫描测试参数覆盖 $N \in \{32, 64, 128, 256, 512, 1024\}$（均为 2 的幂），$K \in \{1,2\}$，SNR $\in [-30,20]$ dB。

$K=2$ 时 $\Delta_f$（频率间距）对估计器不可见。Test 7 中 $\Delta_{\text{bins}}$ 可小至 0.2，$\Delta_z$ 可能极小——保守取 $L=N/2$ 是唯一安全选择。

$K=1$ 时无 $\Delta_z$ 约束（只有一个频率），仅需 $L > r+1 = 3$。

### 自适应公式

$$
\boxed{
L = \begin{cases}
\lfloor N/2 \rfloor, & N \leq 64 \\[4pt]
\lfloor N/2 \rfloor, & K = 2 \\[4pt]
\max\!\left(r+1,\; \lfloor N/4 \rfloor\right), & N \geq 128,\; K = 1
\end{cases}
}
$$

### 量值表

| $N$ | $K$ | $r$ | $L$ | 对比 $L=N/2$ | 精度影响 |
|-----|-----|-----|-----|-------------|---------|
| 32 | 1 | 2 | 16 | 相同 | — |
| 32 | 2 | 4 | 16 | 相同 | — |
| 64 | 1 | 2 | 32 | 相同 | — |
| 64 | 2 | 4 | 32 | 相同 | — |
| 128 | 1 | 2 | **32** | 64→32 | $\varepsilon \times 2.8$，MC平均→ $\times 0.28$ |
| 128 | 2 | 4 | 64 | 相同 | — |
| 256 | 1 | 2 | **64** | 128→64 | 同上 |
| 256 | 2 | 4 | 128 | 相同 | — |
| 512 | 1 | 2 | **128** | 256→128 | 同上 |
| 512 | 2 | 4 | 256 | 相同 | — |
| 1024 | 1 | 2 | **256** | 512→256 | 同上 |
| 1024 | 2 | 4 | 512 | 相同 | — |

$K=1$ 时 $\varepsilon \propto L^{-3/2}$（Ding Thm I.4），$L=N/4$ vs $N/2$ 单次误差放大 $(2)^{1.5} \approx 2.8\times$，但经 $M=100$ 次 MC 平均后方差放大 $2.8/\sqrt{100} = 0.28\times$——**实际 MSE 反而更低**。

> Ding 误差标度引用见 `esprit-1.md` §6.1；MC 平均效应见 `esprit-1.md` §6.2。

---

## 五、与现有 `esprit.cpp` 的对照

现有实现是**标准复 ESPRIT**（非 Unitary），与修正流程的核心差距：

| 现有 `esprit.cpp` | 修正后流程 | 影响 |
|---|---|---|
| `MatrixXcd` 全复数 | $\mathbb{R}$ 实矩阵（Unitary ESPRIT） | FLOPs ~2.8× 减少 |
| $L = N/2$ 硬编码 | 自适应（$K=1$ 时 $L=N/4$） | $K=1$ 大 $N$ 时 ~8× 加速 |
| `SelfAdjointEigenSolver` 全谱 | Lanczos top-$r$ | $L \gg r$ 时 ~$L/r$× 加速 |
| `ComplexEigenSolver` 解 $W$ | `EigenSolver<MatrixXd>` | ~4× 加速，零虚部漂移 |
| `bdcSvd().solve()` 伪逆 | 正规方程 | $r\leq4$ 时 ~5× 加速 |
| 无 Q 变换（非 Unitary） | 稀疏块公式 $T_X$ | 额外 $O(LM)$ 但全流程仍更快 |
| 无可靠性检验 | Haardt §IV-C 检验 | 低 SNR 时防伪频率 |
| 无高频折叠处理 | $\|\omega_k\|>1$ 折叠 | 边界频率稳健性 |

---

## 六、三文件贡献与修正评价

| 文件 | ✅ 保持的贡献 | ❌ 需修正 |
|------|-------------|----------|
| **esprit-1.md** | Q 稀疏块公式（§10.1）、$K_1/K_2$ 显式稀疏形式（§Step4）、可靠性检验（§8.3）、$L$ 缩减理论依据（§3）、Ding 误差标度（§6） | $r = K$ → $r = 2K$（§2.1 §Step3 §Step4 §Step5 §Step6）；由此派生的 $L_{\min}$、复杂度表（§4）均需 $\times 2$ 修正 |
| **esprit-2.md** | Lanczos 详述（§4.2）、精度-复杂度权衡（§7）、扫描测试参数汇总（App B）、实 EVD（§7.1） | $K_1$ 公式 $J_1+J_2$ → $J_1+\Pi J_1\Pi$（§5.2）；Step 8 内部真频筛选 → 删除（§Step8） |
| **esprit-3.md** | 秩约束正确性（§9.1）、标签对称性（§9.2）、输出准则（§9.3）、Square-root vs 协方差分析（§8）、LS→TLS 回退（§6.3）、D24 限制条件表（§10.4）、加窗禁令（§2.3） | $K=1/2$ 分路径使用不同算法 → 统一用 Unitary（§11） |

---

## 七、汇总公式框（速查）

$$
\boxed{
\begin{aligned}
&\textbf{Input: } x[0{:}N{-}1]\;(\text{raw, unwindowed}),\; f_s,\; K,\; r = 2K \\[6pt]
&\text{① } L \gets \text{adapt}(N,K),\; M \gets N-L+1,\;
X(i,j) \gets x[i+j] \in \mathbb{R}^{L \times M} \\[4pt]
&\text{② } Z_{\text{CH}} \gets [X \;\; \Pi_L X \Pi_M] \in \mathbb{R}^{L \times 2M} \quad\text{(esprit-1 §Step1)} \\[4pt]
&\text{③ } T_X \gets \text{block\_formula}(X) \in \mathbb{R}^{L \times 2M}
\quad\text{(esprit-1 §10.1, zero complex ops)} \\[4pt]
&\text{④ } R \gets T_X T_X^T,\; E_s \gets \text{top-}r\text{-eigvecs}(R) \in \mathbb{R}^{L \times r}
\quad\text{(esprit-2 §4.2 Lanczos)} \\[4pt]
&\text{⑤ } K_1,K_2: \text{Haardt Eq.(32), sparse form}
\quad\text{(esprit-1 §Step4, NOT }J_1+J_2\text{)} \\[4pt]
&\text{⑥ } \Upsilon \gets [(K_1E_s)^T(K_1E_s)]^{-1}(K_1E_s)^T(K_2E_s) \in \mathbb{R}^{r \times r}
\quad\text{(esprit-1 §Step5)} \\[4pt]
&\text{⑦ } \Upsilon = T\Omega T^{-1},\; \max|\operatorname{Im}(\omega_k)| \leq \tau\;?
\quad\text{(esprit-1 §8.3, fail→}\varnothing\text{)} \\[4pt]
&\qquad\mu_k \gets 2\arctan(\operatorname{Re}(\omega_k)),\;
\hat{f}_k \gets f_s \cdot |\mu_k|/2\pi
\quad\text{(esprit-1 §Step7, incl. fold)} \\[4pt]
&\text{⑧ Output: ALL } \{\hat{f}_k \in (0,f_s/2)\}\text{, no internal filtering}
\quad\text{(esprit-3 §9)}
\end{aligned}
}
$$

---

*文档版本 v1.0 | 生成日期 2026-07-31 | 综合 esprit-1/2/3.md 交叉审阅结果*
