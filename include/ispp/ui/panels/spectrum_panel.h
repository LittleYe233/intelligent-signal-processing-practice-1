#ifndef ISPP_UI_PANELS_SPECTRUM_PANEL_H
#define ISPP_UI_PANELS_SPECTRUM_PANEL_H

#include "ispp/experiment/experiment_runner.h"

#include <optional>

namespace ispp::ui {

/// 频谱面板：时域信号 + 频域幅度谱 + 估计峰值 + 真实频率参考线。
class SpectrumPanel {
public:
    void render(const std::optional<RunResult> &result);

private:
    const double *LastInputSignalData = nullptr;
};

} // namespace ispp::ui

#endif // ISPP_UI_PANELS_SPECTRUM_PANEL_H
