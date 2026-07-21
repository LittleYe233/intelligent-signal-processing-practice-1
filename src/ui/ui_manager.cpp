#include "ispp/ui/ui_manager.h"
#include "ispp/experiment/experiment_runner.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <format>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ispp::ui {

// ---------------------------------------------------------------------------
// GLFW error callback
// ---------------------------------------------------------------------------
static void glfwErrorCallback(int error, const char *description) {
    std::fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
UiManager::UiManager() {
    Config.Signal.SampleRateHz = 1000.0;
    Config.Signal.SampleCount = 256;
    Config.Signal.FrequencyHz = 100.0;
    Config.Signal.Amplitude = 1.0;
    Config.Signal.PhaseRad = 0.0;
    Config.Env.Window.Kind = WindowKind::RECTANGULAR;
    Config.Env.Noise.Distribution = NoiseDistribution::GAUSSIAN;
    Config.Env.Noise.SnrDb = 20.0;
    Config.Env.Interference.DeltaBins = 0.0;
    Config.Env.Interference.Amplitude = 0.5;
    Config.MaxFreqCount = 1;
    Config.MonteCarlo.IterationCount = 100;
    Config.MonteCarlo.BaseSeed = 42;
}

UiManager::~UiManager() {
    if (Running.load())
        Running.store(false);
    if (Worker.joinable())
        Worker.join();
    shutdown();
}

// ---------------------------------------------------------------------------
// GLFW initialization
// ---------------------------------------------------------------------------
void UiManager::initGlfw() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        Log.log("ERROR: Failed to initialize GLFW");
        std::abort();
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    // Window size = 85% of primary monitor (logical pixels)
    // glfwGetVideoMode returns physical resolution; divide by monitor
    // content scale to obtain logical pixels before applying 85%.
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    float monitor_scale_x = 1.0f, monitor_scale_y = 1.0f;
    glfwGetMonitorContentScale(monitor, &monitor_scale_x, &monitor_scale_y);
    int win_w = static_cast<int>(static_cast<double>(mode->width) /
                                 monitor_scale_x * 0.85);
    int win_h = static_cast<int>(static_cast<double>(mode->height) /
                                 monitor_scale_y * 0.85);
    std::cout << std::format("win=({}, {}) monitor_scale=({}, {})\n", win_w,
                             win_h, monitor_scale_x, monitor_scale_y);

    Window = glfwCreateWindow(win_w, win_h,
                              "ISPPracticeOne — Signal "
                              "Frequency Estimation Simulation",
                              nullptr, nullptr);
    if (!Window) {
        Log.log("ERROR: Failed to create GLFW window");
        glfwTerminate();
        std::abort();
    }

    glfwMakeContextCurrent(Window);
    glfwSwapInterval(0); // no vsync
}

// ---------------------------------------------------------------------------
// ImGui / ImPlot initialization
// ---------------------------------------------------------------------------
void UiManager::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // DPI scaling
    float xscale, yscale;
    glfwGetWindowContentScale(Window, &xscale, &yscale);
    std::cout << std::format("win_scale=({}, {})\n", xscale, yscale);
    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(xscale);

    // Chinese font (Microsoft YaHei)
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\msyh.ttc)", 16.0f);
    ImGui::GetStyle().FontScaleMain = 1.5;

    // ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(Window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------
void UiManager::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    if (Window) {
        glfwDestroyWindow(Window);
        Window = nullptr;
    }
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// Start experiment in background thread
// ---------------------------------------------------------------------------
void UiManager::startExperiment(std::shared_ptr<IEstimator> estimator,
                                std::vector<std::shared_ptr<IMetric>> metrics) {
    if (Running.load())
        return;

    ExperimentConfig config = Config;
    Running.store(true);
    Progress.store(0.0f);
    Log.log("Starting experiment...");

    Worker = std::thread([this, config, est = std::move(estimator),
                          mets = std::move(metrics)]() {
        try {
            ExperimentRunner runner(config, est, mets);
            RunResult result =
                runner.run([this](float p) { Progress.store(p); });

            {
                std::scoped_lock lock(ResultMutex);
                PendingResult = std::move(result);
            }
        } catch (const std::exception &e) {
            Log.log(std::string("ERROR: ") + e.what());
        } catch (...) {
            Log.log("ERROR: Unknown exception in experiment runner.");
        }
        Running.store(false);
    });
    Worker.detach();
}

// ---------------------------------------------------------------------------
// Poll experiment completion
// ---------------------------------------------------------------------------
void UiManager::pollExperiment() {
    std::scoped_lock lock(ResultMutex);
    if (PendingResult) {
        LastResult = std::move(*PendingResult);
        PendingResult.reset();
        Log.log("Experiment completed.");
    }
}

// ---------------------------------------------------------------------------
// Menu bar
// ---------------------------------------------------------------------------
void UiManager::renderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Quit", "Alt+F4"))
                glfwSetWindowShouldClose(Window, GLFW_TRUE);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void UiManager::run() {
    initGlfw();
    initImGui();
    Log.log("UI initialized.");

    while (!glfwWindowShouldClose(Window)) {
        try {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            renderMainMenuBar();

            // ---- Run request handling ----
            std::shared_ptr<IEstimator> pending_est;
            std::vector<std::shared_ptr<IMetric>> pending_mets;
            RunState state;
            state.Running = Running.load();
            state.Progress = Progress.load();

            ConfigPanel.render(Config, state, pending_est, pending_mets);

            if (state.Pending && !state.Running)
                startExperiment(std::move(pending_est),
                                std::move(pending_mets));

            SpectrumPanel.render(LastResult);
            ResultsPanel.render(LastResult);
            Log.render();

            // Check for completed experiment
            pollExperiment();

            // ---- Render ----
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(Window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(Window);
        } catch (const std::exception &e) {
            Log.log(std::string("ERROR in main loop: ") + e.what());
        } catch (...) {
            Log.log("ERROR in main loop: unknown exception.");
        }
    }

    shutdown();
}

} // namespace ispp::ui
