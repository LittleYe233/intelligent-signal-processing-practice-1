#ifndef ISPP_CORE_PEAK_FINDER_H
#define ISPP_CORE_PEAK_FINDER_H

#include <concepts>
#include <cstddef>
#include <span>
#include <vector>

namespace ispp {

/// @brief 通用一维数据寻峰模板工具。
///
/// 与具体物理量解耦，通过中值滤波底噪估计 + 拓扑突出度 + FWHM 筛选
/// 获得鲁棒的峰值检测。所有方法均为 `static`（纯函数工具类）。
///
/// @tparam T 浮点类型（double / float / long double），约束为
/// std::floating_point。
template <std::floating_point T> class PeakFinder {
public:
    /// 寻峰结果。仅承载算法输出（下标 + 突出度），
    /// 调用方可按需通过 `data[peak.Index]` 取回幅值。
    struct Peak {
        std::size_t Index;
        T Prominence;
    };

    /// @brief 寻找峰值的唯一公开接口。
    ///
    /// 流水线（详见 development_solution.md §5.5.3）：
    ///   中值滤波底噪 → 候选局部极大值 → margin 阈值
    ///   → prominence ≥ min_prominence → FWHM ≥ min_width
    ///
    /// @param data           输入的一维线性连续数据视图。
    /// @param kernel_size    中值滤波窗口大小（建议奇数，如 31）。
    /// @param margin         加在底噪（中值滤波结果）之上的固定安全裕度
    ///                       （线性阈值偏移，与 data 同量纲）。
    /// @param min_prominence 峰值最小突出度（线性幅值阈值）。
    /// @param min_width      主瓣最小半高全宽（FWHM），以下标间距为单位，
    ///                       默认 1.0。使用局部线性插值获得亚下标精度。
    /// @return 满足全部条件的峰值数组，按下标升序。
    static std::vector<Peak> findPeaks(std::span<const T> data,
                                       std::size_t kernel_size, T margin,
                                       T min_prominence,
                                       T min_width = static_cast<T>(1.0));

private:
    /// @brief 滑窗中值滤波，作为底噪估计。
    ///
    /// 窗口在边界处自动收缩（不对称，不超过实际边界），不补零。
    /// 返回与 data 等长的数组。
    static std::vector<T> calcMedianFilter(std::span<const T> data,
                                           std::size_t kernel_size);

    /// @brief 计算峰值突出度（topographic prominence）。
    ///
    /// 左右各向外搜索至第一个高度不低于当前峰的参考位置，
    /// 取两侧所有鞍点中较高者：prominence = data[peak] - max(left, right)。
    static T calcProminence(std::span<const T> data, std::size_t peak_idx);

    /// @brief 计算半高全宽（FWHM）。
    ///
    /// 以 prominence 半高为阈值做左右交叉点搜索，
    /// 使用局部线性插值获得亚下标精度。
    /// @return 宽度值，单位为下标间距。
    static T calcWidth(std::span<const T> data, std::size_t peak_idx,
                       T prominence);
};

} // namespace ispp

#include "ispp/core/peak_finder.tpp"

#endif // ISPP_CORE_PEAK_FINDER_H
