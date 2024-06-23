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

    // Get the vulkan information as much as we can in anther thread while we initialize the win32 window
    auto vk_pre_surface = Atelier::VkPreSurfaceInfo();
    auto pre_surface_thread = Atelier::VkPreSurfaceInfo::kick_worker(vk_pre_surface);

    // A place to keep all of the information in the main thread
    auto main_window = Atelier::Window();
    if (Atelier::Window::create_main_window(&main_window, instance_handle) != Atelier::k_success) {
        Atelier::Log::error("Failed when constructing main window");
        return -1;
    }
    ShowWindow(main_window.window_handle, n_cmd_show);

    // Window is shown, so we can get the pre-surface information and combine it with the newly formed window
    // surface to get the surface information we need. From there, we will have ALL information we need to allow
    // the user to create a vulkan instance whenever they want to
    if (pre_surface_thread.joinable()) pre_surface_thread.join();

    // Keep a secondary window around for testing
    auto sub_window = Atelier::Window();
    Atelier::Window::create_sub_window(&sub_window, instance_handle);
    ShowWindow(sub_window.window_handle, n_cmd_show);

    // Next enter into the windowing loop. In order to stop us from blocking the main thread, I like to do the peak
    // message instead
    while (main_window.should_continue) {
        MSG out_msg;
        BOOL peak_res = PeekMessageW(&out_msg, nullptr, 0, 0, PM_REMOVE);
        if (peak_res == 0) continue;  // No messages available

        // Have we received the demand to quit? Either from the OS or the main window
        if (out_msg.message == WM_QUIT) break;
        TranslateMessage(&out_msg);
        DispatchMessageW(&out_msg);
    }

    MessageBoxA(nullptr, "Okay so far\n", "Atelier", MB_OK);

    return 0;
}
