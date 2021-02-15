#pragma once

#include "pch.h"
#include "Common\DeviceResources.h"
#include "animMain.h"


namespace anim
{
    class App
    {
    public:
        static App& getInstance();
        int run(HINSTANCE hInstance, int nCmdShow);

    private:
        App();
        App(const App&) = delete;
        App &operator=(const App&) = delete;

        HRESULT InitializeWindow(HINSTANCE hInstance, int nCmdShow);
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        
        void OnWindowSizeChanged(const DX::Size& newSize);

        std::shared_ptr<DX::DeviceResources> m_deviceResources;
        std::unique_ptr<AnimMain> m_main;
        HWND m_hwnd;
    };
}
