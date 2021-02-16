#include "pch.h"
#include "App.h"

#include <stdexcept>
#include <windows.h>

using namespace anim;

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow)
{
    try
    {
        App& app = App::getInstance();
        return app.run(hInstance, nCmdShow);
    }
    catch (const std::exception& e)
    {
        MessageBox(NULL, e.what(), "Error", MB_OK);
    }
    return 1;
}
