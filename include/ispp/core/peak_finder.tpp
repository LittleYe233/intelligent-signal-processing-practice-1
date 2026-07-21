/// @file peak_finder.tpp
/// @brief Template implementation of PeakFinder.

#ifndef ISPP_CORE_PEAK_FINDER_TPP
#define ISPP_CORE_PEAK_FINDER_TPP

// Try to supress clang error below.
#pragma once

// Include the parent header unconditionally. The include guard in peak_finder.h
// prevents circular inclusion: when already inside peak_finder.h → .tpp, the
// #include is a no-op. This also satisfies clangd/clang-tidy when parsing .tpp
// as a standalone file, because PeakFinder<T> is declared before it's used.
#include "ispp/core/peak_finder.h"
#include <algorithm>
#include <vector>

namespace ispp {

// ============================================================================
// PeakFinder<T>::findPeaks
// ============================================================================

template <std::floating_point T>
std::vector<typename PeakFinder<T>::Peak>
PeakFinder<T>::findPeaks(std::span<const T> data, std::size_t kernel_size,
                         T margin, T min_prominence, T min_width) {
    std::vector<Peak> result;
    const std::size_t N = data.size();

    if (N < 3)
        return result;

    // 1. 中值滤波计算动态底噪包络
    std::vector<T> noise_floor = calcMedianFilter(data, kernel_size);

    // 2. 遍历寻找候选峰值并进行多级筛选
    for (std::size_t i = 1; i < N - 1; ++i) {
        // 基础条件：严格的局部极大值
        if (data[i] > data[i - 1] && data[i] > data[i + 1]) {
            // 筛选 1: 必须超越 "动态底噪 + 安全裕度"
            if (data[i] < noise_floor[i] + margin) {
                continue;
            }
            T prominence = calcProminence(data, i);
            // 筛选 2: 突出度门限约束
            if (prominence < min_prominence) {
                continue;
            }
            // 提取 主瓣宽度 (Half-prominence width)
            T width = calcWidth(data, i, prominence);
            // 筛选 3: 宽度门限约束，过滤瞬态毛刺
            if (width < min_width) {
                continue;
            }

            result.push_back({i, prominence});
        }
    }

    return result;
}

// ============================================================================
// PeakFinder<T>::calcMedianFilter
// ============================================================================

template <std::floating_point T>
std::vector<T> PeakFinder<T>::calcMedianFilter(std::span<const T> data,
                                               std::size_t kernel_size) {
    std::vector<T> result(data.size());
    std::size_t half_k = kernel_size / 2;

    for (std::size_t i = 0; i < data.size(); ++i) {
        std::size_t start = (i > half_k) ? i - half_k : 0;
        std::size_t end = std::min(data.size(), i + half_k + 1);
        std::vector<T> window(data.begin() + static_cast<std::ptrdiff_t>(start),
                              data.begin() + static_cast<std::ptrdiff_t>(end));
        auto mid = window.begin() + window.size() / 2;
        std::ranges::nth_element(window, mid);
        result[i] = *mid;
    }
    return result;
}

// ============================================================================
// PeakFinder<T>::calcProminence
// ============================================================================

template <std::floating_point T>
T PeakFinder<T>::calcProminence(std::span<const T> data, std::size_t peak_idx) {
    T peak_val = data[peak_idx];

    // 向左寻找更高峰或谷底
    T left_min = peak_val;
    for (std::ptrdiff_t j = static_cast<std::ptrdiff_t>(peak_idx) - 1; j >= 0;
         --j) {
        const auto UJ = static_cast<std::size_t>(j);
        if (data[UJ] > peak_val)
            break; // 遇到更高的峰，停止
        if (data[UJ] < left_min)
            left_min = data[UJ];
    }

    // 向右寻找更高峰或谷底
    T right_min = peak_val;
    for (std::size_t j = peak_idx + 1; j < data.size(); ++j) {
        if (data[j] > peak_val)
            break; // 遇到更高的峰，停止
        if (data[j] < right_min)
            right_min = data[j];
    }

    // 突起基底为左右两侧谷底的较高者
    T base = std::max(left_min, right_min);
    return peak_val - base;
}

// ============================================================================
// PeakFinder<T>::calcWidth
// ============================================================================

template <std::floating_point T>
T PeakFinder<T>::calcWidth(std::span<const T> data, std::size_t peak_idx,
                           T prominence) {
    T half_height = data[peak_idx] - prominence / static_cast<T>(2.0);

    // 向左插值寻找半高交点
    T left_pos = static_cast<T>(peak_idx);
    for (std::ptrdiff_t j = static_cast<std::ptrdiff_t>(peak_idx) - 1; j >= 0;
         --j) {
        const auto UJ = static_cast<std::size_t>(j);
        if (data[UJ] < half_height) {
            T diff = data[UJ + 1] - data[UJ];
            if (diff > static_cast<T>(0)) {
                T frac = (half_height - data[UJ]) / diff;
                left_pos = static_cast<T>(j) + frac;
            } else {
                left_pos = static_cast<T>(j);
            }
            break;
        }
        if (j == 0)
            left_pos = static_cast<T>(0); // 触及左侧硬边界
    }

    // 向右插值寻找半高交点
    T right_pos = static_cast<T>(peak_idx);
    for (std::size_t j = peak_idx + 1; j < data.size(); ++j) {
        if (data[j] < half_height) {
            T diff = data[j - 1] - data[j];
            if (diff > static_cast<T>(0)) {
                T frac = (data[j - 1] - half_height) / diff;
                right_pos = static_cast<T>(j - 1) + frac;
            } else {
                right_pos = static_cast<T>(j);
            }
            break;
        }
        if (j == data.size() - 1)
            right_pos = static_cast<T>(data.size() - 1); // 触及右侧硬边界
    }

    return right_pos - left_pos;
}

} // namespace ispp

#endif // ISPP_CORE_PEAK_FINDER_TPP
