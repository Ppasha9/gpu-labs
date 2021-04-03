#pragma once

#include "DirectXHelper.h"

namespace DX
{
    struct Size
    {
        float height;
        float width;

        bool operator==(const Size& other) const
        {
            return height == other.height && width == other.width;
        }

        bool operator!=(const Size& other) const
        {
            return !(*this == other);
        }

        Size &operator=(const Size &other)
        {
            width = max(other.width, 1);
            height = max(other.height, 1);

            return *this;
        }

        Size(float height, float width) : width(max(width, 1)), height(max(height, 1))
        {
        }

        Size(const Size &other)
        {
            *this = other;
        }
    };

    struct RenderTargetTexture
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        D3D11_VIEWPORT viewport;
        CD3D11_TEXTURE2D_DESC textureDesc;
    };

    // Controls all the DirectX device resources.
    class DeviceResources
    {
    public:
        DeviceResources();
        void SetWindow(HWND hwnd);
        void SetLogicalSize(Size logicalSize);
        void Present();

        // Create texture of given size and bind it as render target and shader resource
        RenderTargetTexture createRenderTargetTexture(const Size &size,
            const std::string &namePrefix, UINT mipLevels = 1) const;

        Microsoft::WRL::ComPtr<ID3D11PixelShader> createPixelShader(
            const std::string &namePrefix) const;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> createVertexShader(
            const std::string &namePrefix) const;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> createTexture2D(
            const D3D11_TEXTURE2D_DESC &desc, const std::string &namePrefix,
            const D3D11_SUBRESOURCE_DATA *initData = nullptr) const;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> createShaderResourceView(
            Microsoft::WRL::ComPtr<ID3D11Texture2D> texture,
            const std::string &namePrefix,
            const D3D11_SHADER_RESOURCE_VIEW_DESC *desc = nullptr) const;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> createRenderTargetView(
            Microsoft::WRL::ComPtr<ID3D11Texture2D> texture,
            const std::string &namePrefix) const;

        Microsoft::WRL::ComPtr<ID3D11Buffer> createIndexBuffer(
            const std::vector<unsigned short> &indices,
            const std::string &namePrefix) const;

        template <typename Vertex>
        Microsoft::WRL::ComPtr<ID3D11Buffer> createVertexBuffer(
            const std::vector<Vertex> &vertices,
            const std::string &namePrefix) const
        {
            Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
  
            CD3D11_BUFFER_DESC vertexBufferDesc(
                (UINT)vertices.size() * sizeof(Vertex),
                D3D11_BIND_VERTEX_BUFFER
            );

            D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
            vertexBufferData.pSysMem = vertices.data();

            ThrowIfFailed(
                m_d3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &vertexBuffer)
            );
            SetName(vertexBuffer, namePrefix + "VertexBuffer");

            return vertexBuffer;
        }

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
        ID3D11SamplerState * const * GetSamplerState() const { return m_samplerState.GetAddressOf(); }

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
        Microsoft::WRL::ComPtr<ID3D11SamplerState>     m_samplerState;

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