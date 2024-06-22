#include <Windows.h>
#include "atelier/atelier.h"

int wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR pCmdLine, _In_ int nCmdShow)
{
    MessageBoxA(nullptr, "Hello, World!\n", "Atelier", MB_OK);
}
