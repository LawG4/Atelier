#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>
#include "vulkan/vulkan_core.h"

namespace Atelier
{
typedef uint32_t result;
static constexpr result k_success = 0;

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

struct MainWindow {
    MainWindow() = default;
    bool should_continue = true;
    HINSTANCE win32_app_instance = nullptr;
    HWND win32_main_window = nullptr;
};

/**
 * @brief We get as much information as we physically can about the vulkan devices connected while the surface is
 * getting constructed
 */
struct VkPreSurfaceInfo {
    VkPreSurfaceInfo() = default;
    std::mutex mut;
    uint32_t max_api_ver = 0;
    std::vector<VkExtensionProperties> extensions;
    std::vector<VkLayerProperties> layers;
    VkInstance instance = VK_NULL_HANDLE;
    struct PerDevice {
        PerDevice() = default;
        VkPhysicalDevice device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties props = {};
        std::vector<VkExtensionProperties> extensions;
    };
    std::vector<PerDevice> devices;

    static std::thread kick_worker(VkPreSurfaceInfo& info);
};
}  // namespace Atelier
