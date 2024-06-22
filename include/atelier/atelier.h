#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windef.h>

namespace Atelier
{
struct MainWindow {
    MainWindow() = default;
    bool should_continue = true;
    HINSTANCE win32_app_instance = nullptr;
    HWND win32_main_window = nullptr;
};
}  // namespace Atelier
