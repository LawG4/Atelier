#include "atelier/atelier.h"

static ATOM main_wc_atom = 0;
static ATOM sub_wc_atom = 0;

static LRESULT main_class_proc_func(HWND window_handle, UINT msg_id, WPARAM w_param, LPARAM l_param)
{
    switch (msg_id) {
        // This is our MAIN window, and so when the user closes it, we should tell all other objects that it's time
        // to exit
        case WM_DESTROY:
            Atelier::Log::info("Main window exit clicked");
            PostQuitMessage(0);
            break;
        default:
            break;
    }

    return DefWindowProcW(window_handle, msg_id, w_param, l_param);
}
static LRESULT sub_class_proc_func(HWND window_handle, UINT msg_id, WPARAM w_param, LPARAM l_param)
{
    return DefWindowProcW(window_handle, msg_id, w_param, l_param);
}

Atelier::result Atelier::Window::register_window_classes(HINSTANCE instance_handle)
{
    // Register main class
    WNDCLASSW wc = {};
    wc.hInstance = instance_handle;
    wc.lpszClassName = Window::k_main_class_name;
    wc.lpfnWndProc = main_class_proc_func;
    main_wc_atom = RegisterClassW(&wc);
    if (main_wc_atom == 0) {
        Log::error("Failed to register main window class");
        return -1;
    }

    // Register sub main
    wc.lpszClassName = Window::k_sub_class_name;
    wc.lpfnWndProc = sub_class_proc_func;
    sub_wc_atom = RegisterClassW(&wc);
    if (sub_wc_atom == 0) {
        Log::error("Failed to register main window class");
        return -2;
    }

    Log::info("Success registering the window classes");
    return k_success;
}

Atelier::result Atelier::Window::create_main_window(Window* out, HINSTANCE instance_handle)
{
    if (out == nullptr) return -1;
    out->instance_handle = instance_handle;

    // Create the window
    out->window_handle =
      CreateWindowExW(0, Atelier::Window::k_main_class_name, L"Atelier", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, instance_handle, (void*)out);
    if (out->window_handle == nullptr) {
        Log::error("Failed to create a main window");
        return -1;
    }

    Log::info("Success creating a main window");
    return Atelier::k_success;
}

Atelier::result Atelier::Window::create_sub_window(Window* out, HINSTANCE instance_handle)
{
    if (out == nullptr) return -1;
    out->instance_handle = instance_handle;

    // Create the window
    out->window_handle =
      CreateWindowExW(0, Atelier::Window::k_sub_class_name, L"Atelier", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, instance_handle, (void*)out);
    if (out->window_handle == nullptr) {
        Log::error("Failed to create a sub window");
        return -1;
    }

    Log::info("Success creating a sub window");
    return Atelier::k_success;
}
