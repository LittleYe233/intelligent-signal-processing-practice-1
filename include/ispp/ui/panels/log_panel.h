#ifndef ISPP_UI_PANELS_LOG_PANEL_H
#define ISPP_UI_PANELS_LOG_PANEL_H

#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace ispp::ui {

/// 应用内日志面板，替代被 WIN32_EXECUTABLE 关闭的控制台。
class LogPanel {
public:
    static constexpr std::size_t MAX_LOG = 200;

    void log(std::string_view msg);
    void render();
    void clear();

private:
    std::vector<std::string> Messages;
    std::vector<std::string> RenderCopy; // thread-safe copy for rendering
    std::size_t NextIdx = 0;
    bool Wrapped = false;
    std::mutex Mutex;
};

} // namespace ispp::ui

#endif // ISPP_UI_PANELS_LOG_PANEL_H
