# ESPRIT 算法分析 — 面向本项目需求的低复杂度实现

> **本文档用途**：记录两篇 ESPRIT 论文中与本项目相关的核心结论、数学推导和复杂度优化策略，供后续开发和调优参考。不替代原始论文。

> **注意**：本文档是论文分析方法论的技术记录，不是代码实现计划。所有公式和引用来自原始论文，所有实现决策需要进一步工程审查。

---

## 引用论文

| 简称 | 完整引用 | 本地路径 |
|------|---------|----------|
| **H95** | M. Haardt and J. A. Nossek, "Unitary ESPRIT: How to Obtain Increased Estimation Accuracy with a Reduced Computational Burden," *IEEE Trans. Signal Processing*, vol. 43, no. 5, pp. 1232–1242, May 1995. | `...\Haardt and Nossek - 1995 - Unitary ESPRIT how to obtain increased estimation accuracy with a reduced computational burde\document.md` |
| **D24** | Z. Ding, E. N. Epperly, L. Lin, and R. Zhang, "The ESPRIT Algorithm under High Noise: Optimal Error Scaling and Noisy Super-Resolution," in *2024 IEEE 65th Annual Symposium on Foundations of Computer Science (FOCS)*, pp. 2344–2366, 2024. DOI: 10.1109/FOCS61266.2024.00137. | `...\Ding et al. - 2024 - The ESPRIT algorithm under high noise optimal error scaling and noisy super-resolution\document.md` |

---

## 1. 项目信号模型适配

### 1.1 本项目的实信号模型

总的物理正弦分量数为 $K$：

- 无干扰：$K=1$
- 目标 + 一个干扰：$K=2$

离散实信号：

$$
x[n] = \sum_{k=1}^{K} A_k \cos(\omega_k n + \varphi_k) + w[n], \quad n = 0,\ldots,N-1,
$$

其中：

$$
\omega_k = \frac{2\pi f_k}{F_s}, \quad 0 < \omega_k < \pi.
$$

### 1.2 复指数展开（关键：ESPRIT 的基础）

把每个实正弦展开为共轭复指数对：

$$
A_k\cos(\omega_k n + \varphi_k) = c_k z_k^n + \overline{c_k}\,\overline{z_k}^{\,n},
$$

$$
z_k = e^{j\omega_k}, \quad c_k = \frac{A_k}{2}e^{j\varphi_k}.
$$

**引用**：D24, Eq. (I.1), p. 2344, 本地 `document.md` L37–49。

因此无噪声信号为：

$$
x[n] = \sum_{k=1}^{K} \left( c_k z_k^n + \overline{c_k}\,\overline{z_k}^{\,n} \right).
$$

### 1.3 模型阶数规则（最关键的正确性前提）

$$
\boxed{r = 2K}
$$

| 物理情形 | $K$ | ESPRIT 子空间秩 $r$ |
|:---------|:-----:|:--------------------:|
| 只有目标频率 | 1 | 2 |
| 目标 + 一个不同频率干扰 | 2 | 4 |

> ⚠️ **不能因为只要目标频率就把秩降成 2。** 若存在干扰，数据包含 4 个复指数根。强行 $r=2$ 会使子空间混叠，直接造成频率偏移。

> **例外**：DC ($\omega=0$) 和 Nyquist ($\omega=\pi$) 的共轭根重合，秩规则需特殊处理；常规频率估计应避开这两个退化位置。

---

## 2. 数据矩阵构造

### 2.1 Hankel 矩阵

$$
X = \begin{bmatrix}
x[0] & x[1] & \cdots & x[P-1] \\
x[1] & x[2] & \cdots & x[P] \\
\vdots & \vdots & \ddots & \vdots \\
x[L-1] & x[L] & \cdots & x[N-1]
\end{bmatrix} \in \mathbb{R}^{L \times P},
$$

满足：

$$
L + P - 1 = N.
$$

**引用**：D24, §I-A, p. 2345, 本地 L119–124（Hankel/Toeplitz 重组）。

无噪声时 rank 分析：

$$
X = V_L(\mathcal{Z})\, \operatorname{diag}(\boldsymbol{c})\, V_P(\mathcal{Z})^\mathsf{T},
$$

$$
\mathcal{Z} = (z_1, \overline{z}_1, \ldots, z_K, \overline{z}_K),
$$

$$
\operatorname{rank}(X) = r = 2K.
$$

### 2.2 推荐矩阵尺寸

$$
L \approx P \approx \frac{N}{2},
$$
同时满足：

$$
L - 1 \ge r, \qquad P \ge r.
$$

若 $N=256$，则 $L \approx 128$，$P \approx 129$；对于 $r=4$ 绰绰有余，且有充分余量处理近频率。

> **说明**：这是一项工程平衡，不是论文指定的唯一最优比例。

### 2.3 窗函数问题

**推荐不使用加窗**的原始时间样本构造 Hankel 矩阵。非矩形窗会破坏指数移位不变关系 $x[n] = c z^n$，导致模型失配。

**依据**：H95 Eq. (8)–(11)，本地 L115–201；D24 Eq. (I.1)，本地 L45–49。

---

## 3. ESPRIT 移位不变核心推导

### 3.1 选择矩阵

最大重叠选择：

$$
J_1 = \begin{bmatrix} I_{L-1} & 0 \end{bmatrix}, \qquad
J_2 = \begin{bmatrix} 0 & I_{L-1} \end{bmatrix}.
$$

满足 centro-symmetry 条件：

$$
J_2 = \Pi_{L-1} J_1 \Pi_L.
$$

**引用**：H95, Eq. (10), p. 1234–1235, 本地 L151–163。

### 3.2 移位不变关系

对于 Vandermonde 矩阵：

$$
J_2 V_L(\mathcal{Z}) = J_1 V_L(\mathcal{Z}) D,
$$
$$
D = \operatorname{diag}(\mathcal{Z}) = \operatorname{diag}(z_1, \overline{z}_1, \ldots, z_K, \overline{z}_K).
$$

若 $E_s \in \mathbb{C}^{L \times r}$ 是信号子空间基，存在可逆 $B$ 使得：

$$
E_s = V_L(\mathcal{Z}) B.
$$

于是：

$$
J_2 E_s = J_1 E_s \Psi, \qquad \Psi = B^{-1} D B.
$$

**引用**：H95, Eq. (11)–(13), p. 1235, 本地 L165–201；D24, Algorithm 1 第 2–5 步, p. 2345, 本地 L163–177。

### 3.3 核心结论

$$
\operatorname{eig}(\Psi) = \{z_1, \overline{z}_1, \ldots, z_K, \overline{z}_K\}.
$$

这就是 ESPRIT 不需要频谱搜索的原因——相位因子直接来自解旋转方程的特征值。

---

## 4. 基础版本：实值 Square-Root ESPRIT

### 4.1 子空间分解

对 $X \in \mathbb{R}^{L \times P}$，只保留前 $r$ 个奇异向量：

$$
X \approx E_s \Sigma_s V_s^\mathsf{T},
$$
$$
E_s = U(:, 1:r).
$$

**引用**：H95, SVD 信号子空间估计, Eq. (14)–(15), p. 1235, 本地 L203–223。

### 4.2 旋转方程（最小二乘）

$$
A = J_1 E_s, \qquad B = J_2 E_s,
$$
$$
\Psi_{\mathrm{LS}} = A^\dagger B = (A^\mathsf{T} A)^{-1} A^\mathsf{T} B.
$$

**引用**：H95, Eq. (16), p. 1235, 本地 L215–223。

> 推荐通过 QR 分解或小型 SVD 求伪逆，避免法方程恶化。对 $r \le 4$ 此代价极小。

### 4.3 频率提取

$$
\lambda_i = \operatorname{eig}(\Psi_{\mathrm{LS}}),
$$
$$
\hat{\omega}_i = \arg(\lambda_i),
$$
$$
\hat{f}_i = \frac{F_s}{2\pi} \left| \operatorname{wrap}_{(-\pi,\pi]}(\hat{\omega}_i) \right|.
$$

对共轭正负频率对去重后得到 $K$ 个正频率候选。

---

## 5. Unitary ESPRIT 分支（针对近频率 / 低 SNR / 相关干扰）

当 $K=2$、频率接近、SNR 低或干扰可能相关时，推荐启用 Unitary ESPRIT 的前后向平均。

### 5.1 前后向增强

$$
Z = \begin{bmatrix} X & \Pi_L \overline{X} \end{bmatrix}.
$$

重排列得 centro-Hermitian 形式：

$$
Z_{\mathrm{CH}} = \begin{bmatrix} X & \Pi_L \overline{X} \Pi_P \end{bmatrix}.
$$

**引用**：H95, Eq. (18)–(20), p. 1235–1236, 本地 L241–273。

### 5.2 实值变换

通过左 $\Pi$-real 酉矩阵：

$$
Q_{2n} = \frac{1}{\sqrt{2}} \begin{bmatrix} I_n & j I_n \\ \Pi_n & -j \Pi_n \end{bmatrix}, \quad
Q_{2n+1} = \frac{1}{\sqrt{2}} \begin{bmatrix} I_n & 0 & j I_n \\ 0^\mathsf{T} & \sqrt{2} & 0^\mathsf{T} \\ \Pi_n & 0 & -j \Pi_n \end{bmatrix}.
$$

**引用**：H95, Eq. (3)–(4), p. 1233, 本地 L61–67。

构造实矩阵：

$$
Y = \mathcal{T}(X) = Q_L^H \begin{bmatrix} X & \Pi_L \overline{X} \Pi_P \end{bmatrix} Q_{2P} \in \mathbb{R}^{L \times 2P}.
$$

**引用**：H95, Eq. (6)–(7), p. 1233–1234, 本地 L85–109。

对本项目的原始实数信号，$\overline{X} = X$。

### 5.3 实值子空间

$$
Y \approx E_s \Sigma_s V_s^\mathsf{T}, \qquad E_s \in \mathbb{R}^{L \times r}.
$$

**引用**：H95, Proposition 1, Eq. (19)–(21), p. 1236, 本地 L257–285。

### 5.4 实值选择矩阵

$$
K_1 = Q_{L-1}^H \left( J_1 + \Pi_{L-1} J_1 \Pi_L \right) Q_L,
$$
$$
K_2 = Q_{L-1}^H j\left( J_1 - \Pi_{L-1} J_1 \Pi_L \right) Q_L.
$$

**引用**：H95, Eq. (32), p. 1238, 本地 L407–421。

均为实矩阵（因括号内矩阵 centro-Hermitian）。

### 5.5 实值旋转方程（LS）

$$
A_u = K_1 E_s, \qquad B_u = K_2 E_s,
$$
$$
\Upsilon_{\mathrm{LS}} = A_u^\dagger B_u.
$$

**引用**：H95, Remark 3 / Eq. (31), p. 1238, 本地 L387–405；Table I 第 3 步, p. 1239, 本地 L468–478。

### 5.6 Cayley 变换 → 物理频率

$$
\nu_i = \operatorname{eig}(\Upsilon),
$$
$$
\phi_i = -\frac{\nu_i - j}{\nu_i + j},
$$
$$
\mu_i = 2 \arctan(\nu_i),
$$
$$
\hat{f}_i = \frac{F_s}{2\pi} \left| 2 \arctan(\nu_i) \right|.
$$

**引用**：H95, Eq. (26)–(28), p. 1237, 本地 L337–359；Eq. (33), p. 1238, 本地 L423–427；Table I 第 4–6 步, p. 1239, 本地 L468–478。

---

## 6. LS 默认 / TLS 按需回退策略

### 6.1 TLS 的形式

$$
\Upsilon_{\mathrm{TLS}} = -W_{12} W_{22}^{-1},
$$

其中 $W_{12}, W_{22}$ 来自 $\mathcal{T}(C_1)$ 的右奇异向量块。

**引用**：H95, Proposition 2, Eq. (23)–(27), p. 1236–1237, 本地 L287–359。

### 6.2 为什么默认 LS

- 在 $r \le 4$ 时，LS 的复杂度 $O(L r^2 + r^3) = O(N)$；
- TLS 额外分解的矩阵 $(L-1) \times 2r$ 也不贵；
- H95 的仿真中，Unitary ESPRIT 的 LS 与 TLS 曲线**几乎重合**；
- H95 明确建议优先使用更便宜的 LS。

**引用**：H95, §V-B, p. 1241, 本地 L524–540, 尤其 L540。

### 6.3 回退策略

1. 默认：$\Upsilon = \Upsilon_{\mathrm{LS}}$。
2. 若可靠性检查（§7）失败：
   - 若用了近似/迭代子空间，先提高精度；
   - 再尝试 TLS；
   - 仍失败则输出"不可靠"，不伪造频率值。

---

## 7. 可靠性检查（Unitary ESPRIT 独有收益）

### 7.1 理论结论

$$
\text{所有 } \nu_i \text{ 为实数} \iff \text{估计相位因子精确位于单位圆上}.
$$

若出现复共轭 $\nu_i$，表示快拍不足、噪声过高、模型阶数错误或子空间不稳定。

**引用**：H95, §IV-C, Eq. (30), p. 1237, 本地 L373–383；Table I 第 5 步, p. 1239, 本地 L468–478。

### 7.2 数值判据（工程近似）

$$
\frac{\max_i |\operatorname{Im} \nu_i|}{\max(1, \max_i |\operatorname{Re} \nu_i|)} \le \tau.
$$

阈值 $\tau$ 是工程参数，非论文指定常数。

---

## 8. 复杂度优化要点

设 $r \le 4$，$L \approx P \approx N/2$。

| 阶段 | 推荐做法 | 复杂度 | 精度影响 |
|:-----|:---------|:------:|:---------|
| 模型阶数 | 直接使用 $r = 2K$ | 常数 | 消除错误模型选择风险 |
| Hankel 构造 | $L \times P$ | $O(N^2)$ 隐式构造 | 无 |
| 子空间分解 | 仅求前 $r$ 个实奇异向量 | 迭代约 $O(q L P)$ | 收敛残差达标时与截断 SVD 一致 |
| Unitary 变换 | $Y = \mathcal{T}(X)$ | $O(L P)$ 实数运算 | 不损失精度，提供可靠性结构 |
| LS | $A_u^\dagger B_u$ | $O(L r^2 + r^3)$ | 极小 |
| 小特征分解 | $r \times r$ | $O(r^3) \le O(64)$ | 极小 |
| 幅度重建 | 跳过 | 节省 $O(N r^2)$ | 对频率输出无影响 |

**引用**：H95, p. 1232, 本地 L21–35（完整 SVD 的 $O(M^3)$ 更新成本）。

### 8.1 精度优先（默认）

对 $Y = \mathcal{T}(X)$ 使用 square-root 直接数据矩阵的前 $r$ 个奇异向量：

$$
Y \approx E_s \Sigma_s V_s^\mathsf{T}.
$$

**优势**：避免协方差矩阵条件数平方，降低舍入和溢出风险。  
**引用**：H95, p. 1232, 本地 L21–25；SVD 形式见 p. 1235, 本地 L203–223。

### 8.2 性能优先（需可靠性检查兜底）

改用协方差：

$$
R_Y = Y Y^\mathsf{T}, \quad \text{或} \quad R_{\mathrm{FB}} = \frac12 \left( X X^\mathsf{H} + \Pi_L \overline{X} X^\mathsf{T} \Pi_L \right).
$$

再取前 $r$ 个特征向量。

**代价**：协方差条件数平方。适用条件：SNR 不低、频率间隔不小、奇异值间隔清晰、有可靠性检查兜底。  
**引用**：H95, Remark 1, Eq. (29), p. 1237, 本地 L367–371。

---

## 9. 关于"只输出目标频率"的秩约束与标签对称性

### 9.1 不能通过降低秩来"忽略干扰"

即使最终只输出目标频率，也不能在存在干扰时设置 $r = 2$。若数据包含 $\{z_0, \overline{z}_0, z_1, \overline{z}_1\}$，总秩为 4。强行 $r = 2$ 得到的是**受干扰的子空间低秩逼近**，而非"纯目标子空间"。

正确流程：

$$
r = 4 \to \{\hat{f}_{\text{candidate},1}, \hat{f}_{\text{candidate},2}\} \to \text{外部规则选出目标}.
$$

### 9.2 目标与干扰的标签交换对称性

若没有额外先验，"目标"和"干扰"具有交换对称性：

$$
(A_0, \omega_0, \varphi_0) \leftrightarrow (A_1, \omega_1, \varphi_1),
$$

观测 $x[n]$ 不变。因此从观测本身无法无条件区分谁是目标。

### 9.3 合理的筛选规则（需模型外信息）

$$
\hat{f}_{\text{target}} = \arg\min_{\hat{f}_i \in \mathcal{F}^+} \operatorname{dist}(\hat{f}_i, \mathcal{B}_{\text{target}}),
$$

其中 $\mathcal{B}_{\text{target}}$ 是已知目标频段或跟踪预测区间。

在纯仿真评估中：在估计完成后用真实频率做匹配评分，但**不应在估计阶段把真值馈入 ESPRIT**。

---

## 10. D24 对本项目的启示与限制

### 10.1 核心理论结果

在 D24 假设下，ESPRIT 位置误差可达：

$$
\operatorname{md}(\hat{\boldsymbol{z}}, \boldsymbol{z}) = \widetilde{O}\left( \frac{r^{3/2} \alpha^3}{\mu_r^3 \Delta_z^{3/2} n^{3/2}} \right).
$$

**引用**：D24, Theorem I.4, Eq. (I.11), p. 2346, 本地 L223–233。

其中关键参数：

- $\alpha$：次高斯噪声尺度；
- $\mu_r$：最弱保留分量强度；
- $\Delta_z$：频率根间的单位圆最小间隔；
- $n$：样本/矩阵维度参数。

### 10.2 频率间隔与最小孔径条件

对两个频率：

$$
\Delta_z = |e^{j\omega_1} - e^{j\omega_2}| = 2 \left| \sin\left(\frac{\omega_1 - \omega_2}{2}\right) \right|.
$$

近频率时 $\Delta_z \approx |\omega_1 - \omega_2| = 2\pi |f_1 - f_2| / F_s$。

稳定性必要条件：

$$
n > 1 + \frac{2\pi}{\Delta_z}.
$$

**引用**：D24, Theorem B.1, p. 2362, 本地 L1161–1169。

对本项目 Hankel 结构，应将此理解为有效孔径 $L$ 需足够大。近频率时无法用低复杂度技巧替代足够长的观测记录。

### 10.3 下界

对高斯噪声，没有任何算法能普遍优于 $\Omega(n^{-3/2})$ 的位置误差阶。

**引用**：D24, Theorem I.7, p. 2346, 本地 L245–249；形式化见 Appendix C, p. 2364, 本地 L1300–1329。

### 10.4 限制与不能直接照搬的原因

| 条件 | D24 要求 | 本项目实际情况 |
|:-----|:---------|:---------------|
| **系数** | 实且正强度 $\mu_i > 0$ | 复系数 $c_k = (A_k/2)e^{j\varphi_k}$ |
| **矩阵** | Hermitian PSD Toeplitz | 实 Hankel（非 Hermitian） |
| **噪声** | 独立零均值次高斯 | 支持多种分布（含 Laplacian、脉冲） |

D24 脚注承认：若系数非实且正，第一步 EVD 应改为 SVD。  
**引用**：D24, p. 2345, 脚注 4, 本地 L153–154。

D24 的主定理基于 PS D Toeplitz 结构，不能无条件照搬到实 Hankel 实现。

**后续方向**：本项目可参考 D24 的 $n^{-3/2}$ 阶作为样本数/噪声/频率间隔关系设计时的理论参考，但不能以同样精度保证要求 ESPRIT 实现。

---

## 11. 最终推荐流程（步骤 1–11）

令 $K \in \{1, 2\}$ 为总物理频率数，$r = 2K$。

1. **使用未加窗的原始时间样本**构造 Hankel 矩阵。  
   - 依据：H95 Eq. (8)–(11)，D24 Eq. (I.1)。

2. 构造 $X \in \mathbb{R}^{L \times P}$，$L + P - 1 = N$，$L \approx P$。

3. 若 $K = 1$、SNR 正常、无明显相干风险：  
   使用基础实值 square-root ESPRIT，取前 $r = 2$ 个奇异向量。

4. 若 $K = 2$、频率接近、低 SNR 或干扰可能相关：  
   构造 $Y = \mathcal{T}(X)$，运行 Unitary ESPRIT，取 $r = 4$。

5. 只求前 $r$ 个子空间向量；不做完整谱分解。  
   迭代/部分 SVD 必须检查残差；失败时回退到完整直接 SVD。

6. 默认求 $\Upsilon_{\mathrm{LS}} = (K_1 E_s)^\dagger K_2 E_s$。

7. 求 $\nu_i = \operatorname{eig}(\Upsilon_{\mathrm{LS}})$，$\mu_i = 2\arctan(\nu_i)$，$f_i = F_s |\mu_i| / 2\pi$。

8. 检查全部 $\nu_i$ 是否近似实数（可靠性检查）。  
   失败时：
   - 先提高子空间求解精度；
   - 再回退 TLS；
   - 仍失败则标记不可靠。

9. 将共轭正负频率配对，得到 $K$ 个正频率候选。

10. **不做幅度重建**。  
    - H95：Table I 第 7 步（信号重建）可跳过。  
    - D24：Algorithm 1 第 8 步（最小二乘幅度）可跳过。  
    - **引用**：H95, p. 1239, 本地 L480–484；D24, p. 2345, 本地 L175–177。

11. 使用外部规则（目标频段 / 跟踪预测 / 仿真匹配）从候选集合选择唯一目标频率。

---

## 关键注意事项

| # | 内容 |
|--:|:-----|
| 1 | 存在干扰时 $r=2$ 是不可用的—数据包含 4 个复指数根，总秩为 4 |
| 2 | 加窗会破坏移位不变关系，构造 Hankel 矩阵时不应加窗 |
| 3 | 目标与干扰在没有外部先验时没有可识别性—ESPRIT 输出无标签频率集合 |
| 4 | D24 的 $n^{-3/2}$ 最优阶保证基于正强度 PSD Toeplitz 模型，不能无条件套用到实 Hankel 实现 |
| 5 | D24 的次高斯噪声假设不覆盖 Laplacian（次指数）和脉冲噪声 |
| 6 | $L \approx N/2$ 是指导性起步值，不是最优证明 |
| 7 | 可靠性检查的数值阈值是工程参数，不是论文指定常数 |
| 8 | Unitary ESPRIT 的前后向平均不等于获得独立加倍快拍—噪声仍是结构化的 |
| 9 | 对 $r \le 4$，真正的复杂度瓶颈在子空间分解，不在 LS/TLS 或特征问题 |
| 10 | 实 Hankel 矩阵的子空间分解完全可以在实数域进行，无需复数运算 |
