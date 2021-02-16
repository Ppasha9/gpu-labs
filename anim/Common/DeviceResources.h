#pragma once

namespace DX
{
    struct Size
    {
        float Height;
        float Width;

        bool operator==(const Size& other) const
        {
            return Height == other.Height && Width == other.Width;
        }

        bool operator!=(const Size& other) const
        {
            return !(*this == other);
        }
    };

    // Controls all the DirectX device resources.
    class DeviceResources
    {
    public:
        DeviceResources();
        void SetWindow(HWND hwnd);
        void SetLogicalSize(Size logicalSize);
        void Present();

        // The size of the render target, in dips.
        Size GetLogicalSize() const { return m_logicalSize; }

        // D3D Accessors.
        ID3D11Device*             GetD3DDevice() const { return m_d3dDevice.Get(); }
        ID3D11DeviceContext*      GetD3DDeviceContext() const { return m_d3dContext.Get(); }
        IDXGISwapChain*           GetSwapChain() const { return m_swapChain.Get(); }
        D3D_FEATURE_LEVEL          GetDeviceFeatureLevel() const { return m_d3dFeatureLevel; }
        ID3D11RenderTargetView*   GetBackBufferRenderTargetView() const { return m_d3dRenderTargetView.Get(); }
        ID3D11DepthStencilView*    GetDepthStencilView() const { return m_d3dDepthStencilView.Get(); }
        D3D11_VIEWPORT             GetScreenViewport() const { return m_screenViewport; }
        ID3DUserDefinedAnnotation* GetAnnotation() const { return m_annotation.Get(); }

        // D2D Accessors.
        ID2D1Factory3*       GetD2DFactory() const { return m_d2dFactory.Get(); }
        ID2D1Device2*        GetD2DDevice() const { return m_d2dDevice.Get(); }
        ID2D1DeviceContext2* GetD2DDeviceContext() const { return m_d2dContext.Get(); }
        ID2D1Bitmap1*        GetD2DTargetBitmap() const { return m_d2dTargetBitmap.Get(); }
        IDWriteFactory3*     GetDWriteFactory() const { return m_dwriteFactory.Get(); }
        IWICImagingFactory2* GetWicImagingFactory() const { return m_wicFactory.Get(); }

    private:
        void CreateDeviceIndependentResources();
        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();

        // Direct3D objects.
        Microsoft::WRL::ComPtr<ID3D11Device>              m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext>       m_d3dContext;
        Microsoft::WRL::ComPtr<IDXGISwapChain>            m_swapChain;
        Microsoft::WRL::ComPtr<ID3DUserDefinedAnnotation> m_annotation;

        // Direct3D rendering objects. Required for 3D.
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_d3dRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_d3dDepthStencilView;
        D3D11_VIEWPORT                                 m_screenViewport;

        // Direct2D drawing components.
        Microsoft::WRL::ComPtr<ID2D1Factory3>       m_d2dFactory;
        Microsoft::WRL::ComPtr<ID2D1Device2>        m_d2dDevice;
        Microsoft::WRL::ComPtr<ID2D1DeviceContext2> m_d2dContext;
        Microsoft::WRL::ComPtr<ID2D1Bitmap1>        m_d2dTargetBitmap;

        // DirectWrite drawing components.
        Microsoft::WRL::ComPtr<IDWriteFactory3>     m_dwriteFactory;
        Microsoft::WRL::ComPtr<IWICImagingFactory2> m_wicFactory;

        // Cached device properties.
        D3D_FEATURE_LEVEL           m_d3dFeatureLevel;
        Size                        m_logicalSize;

        HWND m_hwnd = 0;
    };
}