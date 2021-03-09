#include "pch.h"
#include <comdef.h>

#include "App.h"
#include "Common/DirectXHelper.h"

using namespace anim;

App::App()
{
    m_deviceResources = std::make_shared<DX::DeviceResources>();
    m_main = std::unique_ptr<AnimMain>(new AnimMain(m_deviceResources));

    // Register input devices
    static bool raw_input_initialized = false;
    if (raw_input_initialized == false)
    {
        RAWINPUTDEVICE rid;

        rid.usUsagePage = 0x01; // Mouse
        rid.usUsage = 0x02;
        rid.dwFlags = 0;
        rid.hwndTarget = NULL;

        if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
        {
            MessageBoxW(NULL, L"Failed to register raw input devices.", L"Error", MB_ICONERROR);
            exit(-1);
        }

        raw_input_initialized = true;
    }
}

HRESULT App::InitializeWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Register window class
    const char CLASS_NAME[] = "AnimClass";
    WNDCLASSEX wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = CLASS_NAME;
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    m_hwnd = CreateWindow(CLASS_NAME, "Anim", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    if (!m_hwnd)
        return E_FAIL;

    ShowWindow(m_hwnd, nCmdShow);

    return S_OK;
}

App& App::getInstance()
{
    static App app;

    return app;
}

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    App& app = getInstance();

    switch (uMsg)
    {
    case WM_SIZE:
        app.OnWindowSizeChanged({ (float)HIWORD(lParam), (float)LOWORD(lParam) });
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CHAR:
    {
        const unsigned char ch = static_cast<unsigned char>(wParam);
        if (app.m_main->IsCharsAutoRepeat())
        {
            app.m_main->OnChar(ch);
        }
        else
        {
            const bool wasPressed = lParam & 0x40000000;
            if (!wasPressed)
            {
                app.m_main->OnChar(ch);
            }
        }

        break;
    }
    case WM_KEYDOWN:
    {
        const unsigned char keycode = static_cast<unsigned char>(wParam);
        if (app.m_main->IsKeysAutoRepeat())
        {
            app.m_main->OnKeyPressed(keycode);
        }
        else
        {
            const bool wasPressed = lParam & 0x40000000;
            if (!wasPressed)
            {
                app.m_main->OnKeyPressed(keycode);
            }
        }

        break;
    }
    case WM_KEYUP:
    {
        const unsigned char keycode = static_cast<unsigned char>(wParam);
        app.m_main->OnKeyReleased(keycode);

        break;
    }
    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        app.m_main->OnMouseMove(x, y);

        break;
    }
    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        app.m_main->OnLeftPressed(x, y);

        break;
    }
    case WM_RBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        app.m_main->OnRightPressed(x, y);

        break;
    }
    case WM_MBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        app.m_main->OnMiddlePressed(x, y);

        break;
    }
    case WM_LBUTTONUP:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        app.m_main->OnLeftReleased(x, y);

        break;
    }
    case WM_RBUTTONUP:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        app.m_main->OnRightReleased(x, y);

        break;
    }
    case WM_MBUTTONUP:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        app.m_main->OnMiddleReleased(x, y);

        break;
    }
    case WM_MOUSEWHEEL:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
        {
            app.m_main->OnWheelUp(x, y);
        }
        else if (GET_WHEEL_DELTA_WPARAM(wParam) < 0)
        {
            app.m_main->OnWheelDown(x, y);
        }

        break;
    }
    case WM_INPUT:
    {
        UINT dataSize;
        GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

        if (dataSize > 0)
        {
            std::unique_ptr<BYTE[]> rawdata = std::make_unique<BYTE[]>(dataSize);
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawdata.get(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
            {
                RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawdata.get());
                if (raw->header.dwType == RIM_TYPEMOUSE)
                {
                    app.m_main->OnMouseMoveRaw(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
                }
            }
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam); // Need to call DefWindowProc for WM_INPUT messages
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void App::OnWindowSizeChanged(const DX::Size& newSize)
{
    m_deviceResources->SetLogicalSize(newSize);
    m_main->CreateWindowSizeDependentResources();
}

int App::run(HINSTANCE hInstance, int nCmdShow)
{
    // Create window
    DX::ThrowIfFailed(InitializeWindow(hInstance, nCmdShow));

    // Initialize window dependent resources
    m_deviceResources->SetWindow(m_hwnd);
    m_main->CreateWindowSizeDependentResources();

    // Run the message loop
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            m_main->Update();
            if (m_main->Render())
            {
                m_deviceResources->Present();
            }
        }
    }

    return 0;
}
