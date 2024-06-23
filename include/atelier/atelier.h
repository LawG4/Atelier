#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Atelier
{

/**
 * @brief Stupid cursed logger. Uses cursed static elements, so I'm not sure the best way to share between dynamic
 * loadable elements
 */
struct Log {
    static void init();
    static void shutdown();
    static void unformatted(const char* const msg);
    static void info(const char* const msg, ...);
    static void warn(const char* const msg, ...);
    static void error(const char* const msg, ...);
};
struct MainWindow {
    MainWindow() = default;
    bool should_continue = true;
    HINSTANCE win32_app_instance = nullptr;
    HWND win32_main_window = nullptr;
};
}  // namespace Atelier
