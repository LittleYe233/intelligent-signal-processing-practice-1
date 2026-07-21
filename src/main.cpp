#include "ispp/ui/ui_manager.h"
#include <clocale>
#include <libintl.h>

// Windows has unique logic to locale. Though the project is specific to win32,
// wrap with conditional defines to highlight.
#ifdef _WIN32
#include <array>
#include <cstdlib>
#include <filesystem>
#include <libloaderapi.h>
#include <minwindef.h>
#include <sec_api/stdlib_s.h>
#include <windows.h>
#include <winnls.h>
#include <winnt.h>

void initWindowsLocale() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    // In pwsh env, it may not set these envs.
    if (!std::getenv("LANG") && !std::getenv("LC_ALL")) {
        std::array<wchar_t, LOCALE_NAME_MAX_LENGTH> locale_name = {0};
        if (GetUserDefaultLocaleName(locale_name.data(),
                                     LOCALE_NAME_MAX_LENGTH) > 0) {
            // Get locale wide string like "zh-CN" and convert to "zh_CN"
            std::wstring wname(locale_name.data());
            std::string name(wname.begin(), wname.end());
            const size_t POS = name.find('-');
            if (POS != std::string::npos) {
                name[POS] = '_';
            }
            name += ".UTF-8";
            // Inject env
            _putenv_s("LANG", name.c_str());
        }
    }
}

std::filesystem::path getExecutableDir() {
    std::array<wchar_t, MAX_PATH> buffer;
    DWORD len = GetModuleFileNameW(nullptr, buffer.data(), MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        return std::filesystem::path(buffer.data()).parent_path();
    }
    return std::filesystem::current_path();
}
#endif // _WIN32

int main() {
#ifdef _WIN32
    initWindowsLocale();
    std::filesystem::path exe_dir = getExecutableDir();
    std::filesystem::path locales_dir = exe_dir / "locales";
#else  // _WIN32
    std::filesystem::path locales_dir = "./locales";
#endif // _WIN32

    // Set up i18n
    setlocale(LC_ALL, "");
    bindtextdomain("ui", locales_dir.string().c_str());
    bind_textdomain_codeset("ui", "UTF-8");
    bindtextdomain("con", locales_dir.string().c_str());
    bind_textdomain_codeset("con", "UTF-8");

    // Set up main app
    ispp::ui::UiManager app;
    app.run();
    return 0;
}
