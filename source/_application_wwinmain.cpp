/**
 * @brief Start up the windowing loop and messaging loop of the application
 */
#include <Windows.h>
#include "atelier/atelier.h"

int wWinMain(_In_ HINSTANCE instance_handle, _In_opt_ HINSTANCE pre_instance, _In_ PWSTR p_cmd_line,
             _In_ int n_cmd_show)
{
    // Logger initialization
    Atelier::Log::init();

    // Register the window classes so we can create instances of the different window types
    if (Atelier::Window::register_window_classes(instance_handle) != Atelier::k_success) {
        Atelier::Log::error("Failed to register window class");
        return -1;
    }

    // Fetch as much vulkan information as we can pre window being shown
    auto complete_vk = Atelier::VkCompletedState();
    if (complete_vk.pre_surface_default_init() != Atelier::k_success) {
        Atelier::Log::error("Failed to do vulkan pre surface startup");
        return -1;
    }

    // A place to keep all of the information in the main thread
    auto main_window = Atelier::Window();
    if (Atelier::Window::create_main_window(&main_window, instance_handle) != Atelier::k_success) {
        Atelier::Log::error("Failed when constructing main window");
        return -1;
    }

    // Show the window to the screen, while the animation is playing we can append the additional vulkan stuff
    ShowWindow(main_window.window_handle, n_cmd_show);
    auto& surface = complete_vk.m_surfaces.emplace_back();
    surface.init_from_win32_handles(complete_vk.m_instances[0], instance_handle, main_window.window_handle);

    auto& swap = complete_vk.m_swapchains.emplace_back();
    auto swap_info = Atelier::VkCompletedSwapchain::CreateInfo();
    swap_info.create_default_from_win32(complete_vk.m_devices[0], surface);
    swap.init_from_create_info(swap_info);

    // Next enter into the windowing loop. In order to stop us from blocking the main thread, I like to do the peak
    // message instead. We don't listen to a specific window handle so that we can get all the messages in one go
    while (main_window.should_continue) {
        // Look into that message which might have happened or not
        MSG out_msg;
        BOOL peak_res = PeekMessageW(&out_msg, nullptr, 0, 0, PM_REMOVE);
        if (peak_res == 0) continue;  // No messages available

        // Have we received the demand to quit? Either from the OS or the main window
        if (out_msg.message == WM_QUIT) break;
        TranslateMessage(&out_msg);
        DispatchMessageW(&out_msg);
    }

    // Shut down everything
    complete_vk.shutdown();
    Atelier::Log::shutdown();

    return 0;
}
