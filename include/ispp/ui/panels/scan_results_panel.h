#ifndef ISPP_UI_PANELS_SCAN_RESULTS_PANEL_H
#define ISPP_UI_PANELS_SCAN_RESULTS_PANEL_H

#include "ispp/experiment/scan_test_runner.h"
#include <vector>

namespace ispp::ui {

class ScanResultsPanel {
public:
    ScanResultsPanel() = default;

    void render(const std::vector<ScanTestOutput> &results);

private:
    void renderChart(const ChartResult &chart, int chart_idx);

    // 三种图表样式渲染
    void renderLineWithErrorBands(const ChartResult &chart, int chart_idx);
    void renderGroupedBarsWithError(const ChartResult &chart, int chart_idx);
    void renderMultiLine(const ChartResult &chart, int chart_idx);
};

} // namespace ispp::ui

#endif // ISPP_UI_PANELS_SCAN_RESULTS_PANEL_H
