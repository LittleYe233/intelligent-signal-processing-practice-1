#include "ispp/ui/panels/scan_results_panel.h"
#include <array>
#include <imgui.h>
#include <implot.h>
#include <string>
#include <vector>

namespace ispp::ui {

// ---------------------------------------------------------------------------
// 颜色工具
// ---------------------------------------------------------------------------
static ImU32 getSeriesColorU32(int idx) {
    const auto N = ImPlot::GetColormapSize();
    ImVec4 c = ImPlot::GetColormapColor(idx % N);
    return IM_COL32(
        static_cast<int>(c.x * 255.0f), static_cast<int>(c.y * 255.0f),
        static_cast<int>(c.z * 255.0f), static_cast<int>(c.w * 255.0f));
}

static ImVec4 getSeriesColor(int idx) {
    const auto N = ImPlot::GetColormapSize();
    return ImPlot::GetColormapColor(idx % N);
}

// ---------------------------------------------------------------------------
// 色带颜色：向白色混合（适用于 Light 主题）
// whiteness=0 → 纯色，whiteness=1 → 纯白
// ---------------------------------------------------------------------------
static ImU32 makeBandColor(const ImVec4 &color, float whiteness) {
    const float ONE_MINUS_W = 1.0f - whiteness;
    return IM_COL32(
        static_cast<int>((color.x * ONE_MINUS_W + whiteness) * 255.0f),
        static_cast<int>((color.y * ONE_MINUS_W + whiteness) * 255.0f),
        static_cast<int>((color.z * ONE_MINUS_W + whiteness) * 255.0f),
        static_cast<int>((color.w * ONE_MINUS_W + whiteness) * 255.0f));
}

// //
// ---------------------------------------------------------------------------
// // 判断 Y 轴是否为百分比误差（需要对数标尺）—— 待删除功能
// //
// ---------------------------------------------------------------------------
// static bool isPercentageMetric(const std::string &y_label) {
//     return y_label.find("Percentage Error") != std::string::npos;
// }

// ---------------------------------------------------------------------------
// 离散 X 轴时生成等间距索引 {0, 1, 2, ..., n-1}
// ---------------------------------------------------------------------------
static std::vector<double> makeDiscreteIndices(std::size_t n) {
    std::vector<double> idx(n);
    for (std::size_t i = 0; i < n; ++i) {
        idx[i] = static_cast<double>(i);
    }
    return idx;
}

// ---------------------------------------------------------------------------
// 公共接口
// ---------------------------------------------------------------------------
void ScanResultsPanel::render(const std::vector<ScanTestOutput> &results) {
    if (!ImGui::Begin("Scan Test Results")) {
        ImGui::End();
        return;
    }

    if (results.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                           "No scan results — run scan tests first.");
        ImGui::End();
        return;
    }

    int global_chart_idx = 0;
    for (const auto &test_out : results) {
        for (const auto &chart : test_out.Charts) {
            renderChart(chart, global_chart_idx);
            ++global_chart_idx;
        }
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
// 分派到具体样式
// ---------------------------------------------------------------------------
void ScanResultsPanel::renderChart(const ChartResult &chart, int chart_idx) {
    // 子容器高度 = 绘图高度 400 + 边框(2) + 内边距余量，避免缺像素触发滚动条
    const float CHILD_HEIGHT = 820.0f;

    ImGui::BeginChild(chart.Title.c_str(), ImVec2(0, CHILD_HEIGHT),
                      ImGuiChildFlags_ResizeY);

    switch (chart.Style) {
    case ChartStyle::LINE_WITH_ERROR_BANDS:
        renderLineWithErrorBands(chart, chart_idx);
        break;
    case ChartStyle::GROUPED_BARS_WITH_ERROR:
        renderGroupedBarsWithError(chart, chart_idx);
        break;
    case ChartStyle::MULTI_LINE:
        renderMultiLine(chart, chart_idx);
        break;
    }

    ImGui::EndChild();
    ImGui::Separator();
}

// ---------------------------------------------------------------------------
// Style A: LineWithErrorBands
// ---------------------------------------------------------------------------
void ScanResultsPanel::renderLineWithErrorBands(const ChartResult &chart,
                                                int /*chart_idx*/) {
    const int N = static_cast<int>(chart.XValues.size());

    // 离散 X 轴：用等间距索引代替实际数值
    const bool DISCRETE = chart.IsDiscrete;
    const auto PLOT_X = DISCRETE
                            ? makeDiscreteIndices(static_cast<std::size_t>(N))
                            : std::vector<double>{};
    const double *x_data = DISCRETE ? PLOT_X.data() : chart.XValues.data();

    if (ImPlot::BeginPlot(chart.Title.c_str(), ImVec2(-1, -1))) {
        ImPlot::SetupAxis(ImAxis_X1, chart.XLabel.c_str());

        // 百分比误差使用对数量标（非分贝）—— 待删除功能
        // if (isPercentageMetric(chart.YLabel)) {
        //     ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        // }
        ImPlot::SetupAxis(ImAxis_Y1, chart.YLabel.c_str());

        // 离散时使用类别标签（在等间距索引位置）
        if (DISCRETE && !chart.XLabels.empty()) {
            std::vector<const char *> c_labels;
            c_labels.reserve(chart.XLabels.size());
            for (const auto &lbl : chart.XLabels) {
                c_labels.push_back(lbl.c_str());
            }
            ImPlot::SetupAxisTicks(ImAxis_X1, x_data, N, c_labels.data());
        }

        for (std::size_t si = 0; si < chart.Series.size(); ++si) {
            const auto &s = chart.Series[si];
            if (s.Means.empty())
                continue;

            const bool HAS_DIST = !s.Stds.empty();
            const auto COLOR = getSeriesColorU32(static_cast<int>(si));
            const ImVec4 COLOR_F = getSeriesColor(static_cast<int>(si));
            const bool FIRST_SERIES = (si == 0);

            // 构建 ±std 数组
            const auto SZ = static_cast<std::size_t>(N);
            std::vector<double> mean_plus_std(SZ), mean_minus_std(SZ);
            if (HAS_DIST) {
                for (std::size_t i = 0; i < SZ; ++i) {
                    mean_plus_std[i] = s.Means[i] + s.Stds[i];
                    mean_minus_std[i] = s.Means[i] - s.Stds[i];
                }
            }

            // 从后往前画（浅色 → 中色 → 粗线）
            // 向白色混合（适合 Light 主题背景）

            // min~max 浅色带（只有第一组系列显示图例，避免重复）
            if (HAS_DIST && !s.Mins.empty() && !s.Maxs.empty()) {
                const char *band_label =
                    FIRST_SERIES ? "min\u2013max range" : "##minmax";
                ImPlot::PlotShaded(
                    band_label, x_data, s.Mins.data(), s.Maxs.data(), N,
                    {ImPlotProp_FillColor, makeBandColor(COLOR_F, 0.80f)});
            }

            // ±std 中色带（只有第一组系列显示图例）
            if (HAS_DIST) {
                const char *std_label =
                    FIRST_SERIES ? "mean \u00B1 \u03C3" : "##stdband";
                ImPlot::PlotShaded(
                    std_label, x_data, mean_minus_std.data(),
                    mean_plus_std.data(), N,
                    {ImPlotProp_FillColor, makeBandColor(COLOR_F, 0.50f)});
            }

            // 均值粗线（最上层，纯色，每组的系列名作为图例）
            ImPlot::PlotLine(
                s.Name.c_str(), x_data, s.Means.data(), N,
                {ImPlotProp_LineColor, COLOR, ImPlotProp_LineWeight, 2.0f});
        }

        ImPlot::EndPlot();
    }
}

// ---------------------------------------------------------------------------
// Style B: GroupedBarsWithError
// ---------------------------------------------------------------------------
void ScanResultsPanel::renderGroupedBarsWithError(const ChartResult &chart,
                                                  int /*chart_idx*/) {
    const int N = static_cast<int>(chart.XValues.size());
    const int SERIES_N = static_cast<int>(chart.Series.size());

    if (N == 0 || SERIES_N == 0)
        return;

    // 离散 X 轴：用等间距索引作为组中心
    const bool DISCRETE = chart.IsDiscrete;
    const auto GROUP_X = DISCRETE
                             ? makeDiscreteIndices(static_cast<std::size_t>(N))
                             : std::vector<double>{};
    const double *group_centers =
        DISCRETE ? GROUP_X.data() : chart.XValues.data();

    // 计算柱宽度
    const double GROUP_SPAN = 1.0;
    const double GAP_FRAC = 0.15;
    const double TOTAL_BAR_SPAN = GROUP_SPAN * (1.0 - GAP_FRAC);
    const double BAR_WIDTH = TOTAL_BAR_SPAN / static_cast<double>(SERIES_N + 1);

    if (ImPlot::BeginPlot(chart.Title.c_str(), ImVec2(-1, -1))) {
        ImPlot::SetupAxis(ImAxis_X1, chart.XLabel.c_str());

        // 百分比误差使用对数量标（非分贝）—— 待删除功能
        // if (isPercentageMetric(chart.YLabel)) {
        //     ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        // }
        ImPlot::SetupAxis(ImAxis_Y1, chart.YLabel.c_str());

        // 离散类别标签
        if (DISCRETE && !chart.XLabels.empty()) {
            std::vector<const char *> c_labels;
            c_labels.reserve(chart.XLabels.size());
            for (const auto &lbl : chart.XLabels) {
                c_labels.push_back(lbl.c_str());
            }
            ImPlot::SetupAxisTicks(ImAxis_X1, group_centers, N,
                                   c_labels.data());
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, group_centers[0] - 0.6,
                                group_centers[N - 1] + 0.6, ImPlotCond_Once);

        for (int si = 0; si < SERIES_N; ++si) {
            const auto SI = static_cast<std::size_t>(si);
            const auto &s = chart.Series[SI];
            if (s.Means.empty())
                continue;

            // 计算每根柱的中心位置
            const auto SZ = static_cast<std::size_t>(N);
            std::vector<double> bar_centers(SZ);
            for (std::size_t i = 0; i < SZ; ++i) {
                double offset = (static_cast<double>(si) -
                                 static_cast<double>(SERIES_N - 1) / 2.0) *
                                BAR_WIDTH;
                bar_centers[i] = group_centers[i] + offset;
            }

            const auto COLOR = getSeriesColorU32(si);

            // 柱体
            ImPlot::PlotBars(s.Name.c_str(), bar_centers.data(), s.Means.data(),
                             N, BAR_WIDTH, {ImPlotProp_FillColor, COLOR});

            // 误差须（min~max 不对称误差）
            if (!s.Mins.empty() && !s.Maxs.empty()) {
                std::vector<double> neg_err(SZ), pos_err(SZ);
                for (std::size_t i = 0; i < SZ; ++i) {
                    neg_err[i] = s.Means[i] - s.Mins[i];
                    pos_err[i] = s.Maxs[i] - s.Means[i];
                }
                ImPlot::PlotErrorBars(
                    "##err", bar_centers.data(), s.Means.data(), neg_err.data(),
                    pos_err.data(), N, {ImPlotProp_LineColor, COLOR});
            }
        }

        ImPlot::EndPlot();
    }
}

// ---------------------------------------------------------------------------
// Style C: MultiLine
// ---------------------------------------------------------------------------
void ScanResultsPanel::renderMultiLine(const ChartResult &chart,
                                       int /*chart_idx*/) {
    const int N = static_cast<int>(chart.XValues.size());

    // 离散 X 轴：用等间距索引代替实际数值
    const bool DISCRETE = chart.IsDiscrete;
    const auto PLOT_X = DISCRETE
                            ? makeDiscreteIndices(static_cast<std::size_t>(N))
                            : std::vector<double>{};
    const double *x_data = DISCRETE ? PLOT_X.data() : chart.XValues.data();

    if (ImPlot::BeginPlot(chart.Title.c_str(), ImVec2(-1, -1))) {
        ImPlot::SetupAxis(ImAxis_X1, chart.XLabel.c_str());

        // 百分比误差使用对数量标（非分贝）
        // if (isPercentageMetric(chart.YLabel)) {
        //     ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        // }
        ImPlot::SetupAxis(ImAxis_Y1, chart.YLabel.c_str());

        // 离散时使用类别标签
        if (DISCRETE && !chart.XLabels.empty()) {
            std::vector<const char *> c_labels;
            c_labels.reserve(chart.XLabels.size());
            for (const auto &lbl : chart.XLabels) {
                c_labels.push_back(lbl.c_str());
            }
            ImPlot::SetupAxisTicks(ImAxis_X1, x_data, N, c_labels.data());
        }

        // 不同系列用不同标记类型区分
        static constexpr std::array MARKERS = {
            ImPlotMarker_Circle, ImPlotMarker_Square, ImPlotMarker_Diamond,
            ImPlotMarker_Up};

        for (std::size_t si = 0; si < chart.Series.size(); ++si) {
            const auto &s = chart.Series[si];
            if (s.Means.empty())
                continue;

            const auto COLOR = getSeriesColorU32(static_cast<int>(si));
            const auto MARKER =
                MARKERS[static_cast<std::size_t>(si) % MARKERS.size()];

            ImPlot::PlotLine(s.Name.c_str(), x_data, s.Means.data(), N,
                             {ImPlotProp_LineColor, COLOR,
                              ImPlotProp_LineWeight, 1.5f, ImPlotProp_Marker,
                              MARKER, ImPlotProp_MarkerSize, 4.0f});
        }

        ImPlot::EndPlot();
    }
}

} // namespace ispp::ui
