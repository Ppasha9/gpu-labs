#include "pch.h"
#include "App.h"
#include "Common/DirectXHelper.h"

using namespace anim;

App::App()
{
    m_deviceResources = std::make_shared<DX::DeviceResources>();
    m_main = std::unique_ptr<AnimMain>(new AnimMain(m_deviceResources));
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
