#ifndef ISPP_UI_UI_MANAGER_H
#define ISPP_UI_UI_MANAGER_H

#include "ispp/experiment/experiment_config.h"
#include "ispp/experiment/experiment_runner.h"
#include "ispp/ui/panels/config_panel.h"
#include "ispp/ui/panels/log_panel.h"
#include "ispp/ui/panels/results_panel.h"
#include "ispp/ui/panels/spectrum_panel.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

struct GLFWwindow;

namespace ispp::ui {

/// 应用程序主 UI 管理器。
class UiManager {
public:
    UiManager();
    ~UiManager();
    UiManager(const UiManager &) = delete;
    UiManager &operator=(const UiManager &) = delete;
    UiManager(UiManager &&) = delete;
    UiManager &operator=(UiManager &&) = delete;

    void run();

private:
    // --- 窗口 / GL 状态 ---
    GLFWwindow *Window = nullptr;

    // --- 实验状态 ---
    ExperimentConfig Config;
    std::optional<RunResult> LastResult;

    // --- 运行线程 ---
    std::thread Worker;
    std::atomic<bool> Running{false};
    std::atomic<float> Progress{0.0f};
    std::mutex ResultMutex;
    std::optional<RunResult> PendingResult;

    // --- 面板 ---
    ConfigPanel ConfigPanel;
    SpectrumPanel SpectrumPanel;
    ResultsPanel ResultsPanel;
    LogPanel Log;

    // --- 初始化 / 清理 ---
    void initGlfw();
    void initImGui();
    void shutdown();

    // --- 实验控制 ---
    void startExperiment(std::shared_ptr<IEstimator> estimator,
                         std::vector<std::shared_ptr<IMetric>> metrics);
    void pollExperiment();

    // --- 渲染 ---
    void renderMainMenuBar();
};

} // namespace ispp::ui

#endif // ISPP_UI_UI_MANAGER_H
