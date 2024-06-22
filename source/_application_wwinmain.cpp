/**
 * @brief Start up the windowing loop and messaging loop of the application
 */
#include <Windows.h>
#include "atelier/atelier.h"

LRESULT atelier_proc_function(HWND window_handle, UINT msg_id, WPARAM w_param, LPARAM l_param);
static constexpr wchar_t main_class_name[] = L"Atelier main window class";

int wWinMain(_In_ HINSTANCE instance_handle, _In_opt_ HINSTANCE pre_instance, _In_ PWSTR p_cmd_line,
             _In_ int n_cmd_show)
{
    // A place to keep all of the information in the main thread
    auto main_window = Atelier::MainWindow();
    main_window.win32_app_instance = instance_handle;

    // We're creating a window class
    WNDCLASSW wc = {};
    wc.lpszClassName = main_class_name;
    wc.hInstance = instance_handle;
    wc.lpfnWndProc = atelier_proc_function;
    ATOM wc_atom = RegisterClassW(&wc);
    if (wc_atom == 0) {
        MessageBoxA(nullptr, "Failed to register main window class\n", "Atelier", MB_ICONERROR);
        return -1;
    }

    // Lets create a win32 window!. We attach the atelier main window over to the window class as a user data
    // pointer when constructing the data
    main_window.win32_main_window =
      CreateWindowExW(0, main_class_name, L"Atelier", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                      CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, instance_handle, (void*)&main_window);
    if (main_window.win32_main_window == nullptr) {
        MessageBoxA(nullptr, "Failed to create a main window \n", "Atelier", MB_ICONERROR);
        return -1;
    }
    ShowWindow(main_window.win32_main_window, n_cmd_show);

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
}

LRESULT atelier_proc_function(HWND window_handle, UINT msg_id, WPARAM w_param, LPARAM l_param)
{
    switch (msg_id) {
        // This is our MAIN window, and so when the user closes it, we should tell all other objects that it's time
        // to exit
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            break;
    }

    return DefWindowProcW(window_handle, msg_id, w_param, l_param);
}
