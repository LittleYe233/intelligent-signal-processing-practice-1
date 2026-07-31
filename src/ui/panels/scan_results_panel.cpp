#include "ispp/ui/panels/scan_results_panel.h"
#include "ispp/i18n.h"
#include <array>
#include <cstdio>
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
// 标题本地化：从原子 msgid 分量组合显示标题（仅 UI 线程调用 _UI）。
// chart.Title 仍为英文组合串，仅用作稳定 ImGui/ImPlot ID。
// ---------------------------------------------------------------------------
static std::string localizedTitle(const ChartResult &chart) {
    std::string t = _UI(chart.TestName.c_str());
    t += " — ";
    t += _UI(chart.YLabel.c_str());
    if (!chart.ChartDimLabel.empty()) {
        t += " [";
        t += _UI(chart.ChartDimLabel.c_str());
        t += "]";
    }
    if (chart.IsOverview) {
        t += " (";
        t += _UI("overview");
        t += ")";
    }
    return t;
}

// ---------------------------------------------------------------------------
// 将类别标签本地化为稳定的 std::string 存储。
// 注意：dgettext 返回指向内部静态缓冲区的指针，下一次调用会覆盖，
// 故必须先物化为 std::vector<std::string> 再取 .c_str()。
// ---------------------------------------------------------------------------
static std::vector<const char *>
makeLocalizedLabelPtrs(const std::vector<std::string> &labels,
                       std::vector<std::string> &storage) {
    storage.clear();
    storage.reserve(labels.size());
    for (const auto &lbl : labels) {
        storage.emplace_back(_UI(lbl.c_str()));
    }
    std::vector<const char *> ptrs;
    ptrs.reserve(storage.size());
    for (const auto &lbl : storage) {
        ptrs.push_back(lbl.c_str());
    }
    return ptrs;
}

// ---------------------------------------------------------------------------
// 系列图例标签本地化。
// 逐峰系列（PeakRank > 0）用翻译后的 "Peak %d" 格式生成；其余系列直接翻译
// s.Name。格式串经 _UI() 即时传入 snprintf（gettext
// 静态缓冲区即时消费，安全）。
// ---------------------------------------------------------------------------
static std::string localizedSeriesLabel(const SeriesResult &s) {
    if (s.PeakRank > 0) {
        constexpr std::size_t BUF_SIZE = 32;
        std::array<char, BUF_SIZE> buf{};
        std::snprintf(buf.data(), BUF_SIZE, _UI("Peak %d"), s.PeakRank);
        return buf.data();
    }
    return _UI(s.Name.c_str());
}

// ---------------------------------------------------------------------------
// 图表尺寸：宽度留出右侧余量，避免最右侧 X 轴刻度标签被子容器边框遮挡；
// 高度填满。余量按字号缩放以适配 DPI / 字体放缩。
// ---------------------------------------------------------------------------
static ImVec2 plotSize() {
    constexpr float MIN_PLOT_WIDTH = 100.0f;
    const float MARGIN = ImGui::GetFontSize() * 2.0f;
    const float WIDTH = ImGui::GetContentRegionAvail().x - MARGIN;
    return {WIDTH > MIN_PLOT_WIDTH ? WIDTH : MIN_PLOT_WIDTH, -1.0f};
}

// ---------------------------------------------------------------------------
// 公共接口
// ---------------------------------------------------------------------------
void ScanResultsPanel::render(const std::vector<ScanTestOutput> &results) {
    if (!ImGui::Begin(_UI("Scan Test Results"))) {
        ImGui::End();
        return;
    }

    if (results.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                           _UI("No scan results — run scan tests first."));
        ImGui::End();
        return;
    }

    // 各轴自动拟合时两侧各留 5% 余量（FitPadding=0.1 → 每侧 5% 数据范围）。
    // 仅作用于扫描结果面板，不影响频谱面板。
    ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.1f, 0.1f));

    int global_chart_idx = 0;
    for (const auto &test_out : results) {
        for (const auto &chart : test_out.Charts) {
            renderChart(chart, global_chart_idx);
            ++global_chart_idx;
        }
    }

    ImPlot::PopStyleVar();
    ImGui::End();
}

// ---------------------------------------------------------------------------
// 分派到具体样式
// ---------------------------------------------------------------------------
void ScanResultsPanel::renderChart(const ChartResult &chart, int chart_idx) {
    // 子容器高度 = 绘图高度 400 + 边框(2) + 内边距余量，避免缺像素触发滚动条
    const float CHILD_HEIGHT = 820.0f;

    // chart.Title 为英文组合串，用作稳定唯一的子窗口 ID（不受 locale 影响）。
    ImGui::BeginChild(chart.Title.c_str(), ImVec2(0, CHILD_HEIGHT),
                      ImGuiChildFlags_ResizeY);

    switch (chart.Style) {
    case ChartStyle::LINE_WITH_ERROR_BANDS:
        renderLineWithErrorBands(chart, chart_idx);
        break;
    case ChartStyle::GROUPED_BARS_WITH_ERROR:
        renderGroupedBarsWithError(chart, chart_idx);
        break;
    case ChartStyle::GROUPED_BARS:
        renderGroupedBars(chart, chart_idx);
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

    const std::string TITLE = localizedTitle(chart);
    if (ImPlot::BeginPlot(TITLE.c_str(), plotSize())) {
        ImPlot::SetupAxis(ImAxis_X1, _UI(chart.XLabel.c_str()));
        ImPlot::SetupAxis(ImAxis_Y1, _UI(chart.YLabel.c_str()));

        // 离散时使用类别标签（在等间距索引位置）
        std::vector<std::string> label_storage;
        if (DISCRETE && !chart.XLabels.empty()) {
            const auto C_LABELS =
                makeLocalizedLabelPtrs(chart.XLabels, label_storage);
            ImPlot::SetupAxisTicks(ImAxis_X1, x_data, N, C_LABELS.data());
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
                    FIRST_SERIES ? _UI("min–max range") : "##minmax";
                ImPlot::PlotShaded(
                    band_label, x_data, s.Mins.data(), s.Maxs.data(), N,
                    {ImPlotProp_FillColor, makeBandColor(COLOR_F, 0.80f)});
            }

            // ±std 中色带（只有第一组系列显示图例）
            if (HAS_DIST) {
                const char *std_label =
                    FIRST_SERIES ? _UI("mean ± σ") : "##stdband";
                ImPlot::PlotShaded(
                    std_label, x_data, mean_minus_std.data(),
                    mean_plus_std.data(), N,
                    {ImPlotProp_FillColor, makeBandColor(COLOR_F, 0.50f)});
            }

            // 均值粗线（最上层，纯色，每组的系列名作为图例）
            ImPlot::PlotLine(
                localizedSeriesLabel(s).c_str(), x_data, s.Means.data(), N,
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

    const std::string TITLE = localizedTitle(chart);
    if (ImPlot::BeginPlot(TITLE.c_str(), plotSize())) {
        ImPlot::SetupAxis(ImAxis_X1, _UI(chart.XLabel.c_str()));
        ImPlot::SetupAxis(ImAxis_Y1, _UI(chart.YLabel.c_str()));

        // 离散类别标签
        std::vector<std::string> label_storage;
        if (DISCRETE && !chart.XLabels.empty()) {
            const auto C_LABELS =
                makeLocalizedLabelPtrs(chart.XLabels, label_storage);
            ImPlot::SetupAxisTicks(ImAxis_X1, group_centers, N,
                                   C_LABELS.data());
        }
        // X 轴范围交由自动拟合 + FitPadding(0.1) 统一处理（每侧 5% 余量）。

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
            ImPlot::PlotBars(localizedSeriesLabel(s).c_str(),
                             bar_centers.data(), s.Means.data(), N, BAR_WIDTH,
                             {ImPlotProp_FillColor, COLOR});

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
// Style B2: GroupedBars (no error whiskers)
// ---------------------------------------------------------------------------
void ScanResultsPanel::renderGroupedBars(const ChartResult &chart,
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

    const std::string TITLE = localizedTitle(chart);
    if (ImPlot::BeginPlot(TITLE.c_str(), plotSize())) {
        ImPlot::SetupAxis(ImAxis_X1, _UI(chart.XLabel.c_str()));
        ImPlot::SetupAxis(ImAxis_Y1, _UI(chart.YLabel.c_str()));

        // 离散类别标签
        std::vector<std::string> label_storage;
        if (DISCRETE && !chart.XLabels.empty()) {
            const auto C_LABELS =
                makeLocalizedLabelPtrs(chart.XLabels, label_storage);
            ImPlot::SetupAxisTicks(ImAxis_X1, group_centers, N,
                                   C_LABELS.data());
        }

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

            // 柱体（无误差须）
            ImPlot::PlotBars(localizedSeriesLabel(s).c_str(),
                             bar_centers.data(), s.Means.data(), N, BAR_WIDTH,
                             {ImPlotProp_FillColor, COLOR});
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

    const std::string TITLE = localizedTitle(chart);
    if (ImPlot::BeginPlot(TITLE.c_str(), plotSize())) {
        ImPlot::SetupAxis(ImAxis_X1, _UI(chart.XLabel.c_str()));
        ImPlot::SetupAxis(ImAxis_Y1, _UI(chart.YLabel.c_str()));

        // 离散时使用类别标签
        std::vector<std::string> label_storage;
        if (DISCRETE && !chart.XLabels.empty()) {
            const auto C_LABELS =
                makeLocalizedLabelPtrs(chart.XLabels, label_storage);
            ImPlot::SetupAxisTicks(ImAxis_X1, x_data, N, C_LABELS.data());
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

            ImPlot::PlotLine(
                localizedSeriesLabel(s).c_str(), x_data, s.Means.data(), N,
                {ImPlotProp_LineColor, COLOR, ImPlotProp_LineWeight, 1.5f,
                 ImPlotProp_Marker, MARKER, ImPlotProp_MarkerSize, 4.0f});
        }

        ImPlot::EndPlot();
    }
}

} // namespace ispp::ui
