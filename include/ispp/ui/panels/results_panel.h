#ifndef ISPP_UI_PANELS_RESULTS_PANEL_H
#define ISPP_UI_PANELS_RESULTS_PANEL_H

#include "ispp/experiment/experiment_runner.h"

#include <optional>

namespace ispp::ui {

/// 结果面板：指标表格（mean/std/min/max）+ 计算耗时。
class ResultsPanel {
public:
    void render(const std::optional<RunResult> &result);
};

} // namespace ispp::ui

#endif // ISPP_UI_PANELS_RESULTS_PANEL_H
