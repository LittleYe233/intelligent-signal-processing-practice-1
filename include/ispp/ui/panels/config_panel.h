#ifndef ISPP_UI_PANELS_CONFIG_PANEL_H
#define ISPP_UI_PANELS_CONFIG_PANEL_H

#include "ispp/experiment/experiment_config.h"

#include <array>
#include <memory>
#include <vector>

namespace ispp {

class IEstimator;
class IMetric;

namespace ui {

/// 运行请求状态（UiManager 与 ConfigPanel 共享）。
struct RunState {
    bool Pending = false;
    float Progress = 0.0f;
    bool Running = false;
};

/// 实验配置面板。
class ConfigPanel {
public:
    void render(ExperimentConfig &config, RunState &state,
                std::shared_ptr<IEstimator> &estimator,
                std::vector<std::shared_ptr<IMetric>> &metrics);

private:
    int SelectedAlgorithm = 0;
    std::array<bool, 3> MetricsMask = {true, true, true};
};

} // namespace ui
} // namespace ispp

#endif // ISPP_UI_PANELS_CONFIG_PANEL_H
