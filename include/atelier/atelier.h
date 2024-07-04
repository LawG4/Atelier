#pragma once
#include "atelier_base.h"
#include "atelier_vk.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace Atelier
{

/**
 * @brief Container for a win32 window
 */
struct Window {
    Window() = default;
    static constexpr wchar_t k_main_class_name[] = L"Atelier main-window class";
    static constexpr wchar_t k_sub_class_name[] = L"Atelier sub-window class";

    // Registers both top level and bottom level window classes in win32 operating system
    static result register_window_classes(HINSTANCE instance_handle);

    // Make a top level main window which will exit when the user is done
    static result create_main_window(Window* out, HINSTANCE instance_handle);

    // Make a sub window which doesn't
    static result create_sub_window(Window* out, HINSTANCE instance_handle);

    bool should_continue = true;
    HINSTANCE instance_handle = nullptr;
    HWND window_handle = nullptr;
};

struct StateSingleton {
    std::vector<VkCompletedInstance> vk;
    uint32_t selected_device;
    uint32_t selected_present_queue;
    uint32_t selected_graphics_queue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
};

extern StateSingleton g_atelier;

}  // namespace Atelier
