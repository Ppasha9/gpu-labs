#include "pch.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "roapi.h"

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;

// Constructor for DeviceResources.
DX::DeviceResources::DeviceResources() :
    m_screenViewport(),
    m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
    m_logicalSize(1, 1)
{
    CreateDeviceIndependentResources();
    CreateDeviceResources();
}

// Configures resources that don't depend on the Direct3D device.
void DX::DeviceResources::CreateDeviceIndependentResources()
{
    // Initialize Direct2D resources.
    D2D1_FACTORY_OPTIONS options;
    ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
    // If the project is in a debug build, enable Direct2D debugging via SDK Layers.
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    // Initialize the Direct2D Factory.
    DX::ThrowIfFailed(
        D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory3),
            &options,
            &m_d2dFactory
            )
        );

    // Initialize the DirectWrite Factory.
    DX::ThrowIfFailed(
        DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory3),
            &m_dwriteFactory
            )
        );

    // Initialize the Windows Imaging Component (WIC) Factory.
    Windows::Foundation::Initialize(RO_INIT_SINGLETHREADED);
    DX::ThrowIfFailed(
        CoCreateInstance(
            CLSID_WICImagingFactory2,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_wicFactory)
            )
        );
}

// Configures the Direct3D device, and stores handles to it and the device context.
void DX::DeviceResources::CreateDeviceResources() 
{
    // This flag adds support for surfaces with a different color channel ordering
    // than the API default. It is required for compatibility with Direct2D.
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
    if (DX::SdkLayersAvailable())
    {
        // If the project is in a debug build, enable debugging via SDK Layers with this flag.
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
#endif

    // This array defines the set of DirectX hardware feature levels this app will support.
    // Note the ordering should be preserved.
    // Don't forget to declare your application's minimum required feature level in its
    // description.  All applications are assumed to support 9.1 unless otherwise stated.
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    // Create the Direct3D 11 API device object and a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // Specify nullptr to use the default adapter.
        D3D_DRIVER_TYPE_HARDWARE,    // Create a device using the hardware graphics driver.
        0,                            // Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
        creationFlags,                // Set debug and Direct2D compatibility flags.
        featureLevels,                // List of feature levels this app can support.
        ARRAYSIZE(featureLevels),    // Size of the list above.
        D3D11_SDK_VERSION,            // Always set this to D3D11_SDK_VERSION for Windows Store apps.
        &device,                    // Returns the Direct3D device created.
        &m_d3dFeatureLevel,            // Returns feature level of device created.
        &context                    // Returns the device immediate context.
        );

    if (FAILED(hr))
    {
        // If the initialization fails, fall back to the WARP device.
        // For more information on WARP, see: 
        // https://go.microsoft.com/fwlink/?LinkId=286690
        DX::ThrowIfFailed(
            D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
                0,
                creationFlags,
                featureLevels,
                ARRAYSIZE(featureLevels),
                D3D11_SDK_VERSION,
                &device,
                &m_d3dFeatureLevel,
                &context
                )
            );
    }

    // Store pointers to the Direct3D 11.3 API device and immediate context.
    DX::ThrowIfFailed(
        device.As(&m_d3dDevice)
        );

    DX::ThrowIfFailed(
        context.As(&m_d3dContext)
        );

    // Create annotations
    DX::ThrowIfFailed(
        m_d3dContext->QueryInterface(__uuidof(m_annotation.Get()),
            reinterpret_cast<void**>(m_annotation.GetAddressOf()))
        );

    // Create the Direct2D device object and a corresponding context.
    ComPtr<IDXGIDevice3> dxgiDevice;
    DX::ThrowIfFailed(
        m_d3dDevice.As(&dxgiDevice)
        );

    DX::ThrowIfFailed(
        m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
        );

    DX::ThrowIfFailed(
        m_d2dDevice->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
            &m_d2dContext
            )
        );
}

// These resources need to be recreated every time the window size is changed.
void DX::DeviceResources::CreateWindowSizeDependentResources() 
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews[] = {nullptr};
    m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
    m_d3dRenderTargetView = nullptr;
    m_d2dContext->SetTarget(nullptr);
    m_d2dTargetBitmap = nullptr;
    m_d3dDepthStencilView = nullptr;
    m_d3dContext->Flush();

    if (m_swapChain != nullptr)
    {
        // If the swap chain already exists, resize it.
        DX::ThrowIfFailed(
            m_swapChain->ResizeBuffers(
                2, // Double-buffered swap chain.
                lround(m_logicalSize.width),
                lround(m_logicalSize.height),
                DXGI_FORMAT_B8G8R8A8_UNORM,
                0
                )
            );
    }
    else
    {
        // Otherwise, create a new one using the same adapter as the existing Direct3D device.
        DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };

        swapChainDesc.BufferDesc.Width = lround(m_logicalSize.width);        // Match the size of the window.
        swapChainDesc.BufferDesc.Height = lround(m_logicalSize.height);
        swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;                // This is the most common swap chain format.
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.SampleDesc.Count = 1;                                // Don't use multi-sampling.
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;                                    // Use double-buffering to minimize latency.
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;    // All Windows Store apps must use this SwapEffect.
        swapChainDesc.Flags = 0;
        swapChainDesc.OutputWindow = m_hwnd;
        swapChainDesc.Windowed = TRUE;

        // This sequence obtains the DXGI factory that was used to create the Direct3D device above.
        ComPtr<IDXGIDevice3> dxgiDevice;
        DX::ThrowIfFailed(
            m_d3dDevice.As(&dxgiDevice)
            );

        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(
            dxgiDevice->GetAdapter(&dxgiAdapter)
            );

        ComPtr<IDXGIFactory4> dxgiFactory;
        DX::ThrowIfFailed(
            dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
            );

        DX::ThrowIfFailed(
            dxgiFactory->CreateSwapChain(
                m_d3dDevice.Get(),
                &swapChainDesc,
                &m_swapChain
            )
        );
    }

    // Create a render target view of the swap chain back buffer.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
        );

    DX::ThrowIfFailed(
        m_d3dDevice->CreateRenderTargetView(
            backBuffer.Get(),
            nullptr,
            &m_d3dRenderTargetView
            )
        );
    std::string name = "BackBufferRenderTargetView";
    m_d3dRenderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(),
        name.c_str());

    // Create a depth stencil view for use with 3D rendering if needed.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT, 
        lround(m_logicalSize.width),
        lround(m_logicalSize.height),
        1, // This depth stencil view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_DEPTH_STENCIL
        );

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(
        m_d3dDevice->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            &depthStencil
            )
        );

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(
        m_d3dDevice->CreateDepthStencilView(
            depthStencil.Get(),
            &depthStencilViewDesc,
            &m_d3dDepthStencilView
            )
        );

    // Create sampler state
    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0;
    samplerDesc.BorderColor[1] = 0;
    samplerDesc.BorderColor[2] = 0;
    samplerDesc.BorderColor[3] = 0;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    DX::ThrowIfFailed(
        m_d3dDevice->CreateSamplerState(&samplerDesc, &m_samplerState)
    );

    // Set the 3D rendering viewport to target the entire window.
    m_screenViewport = CD3D11_VIEWPORT(
        0.0f,
        0.0f,
        m_logicalSize.width,
        m_logicalSize.height
        );

    m_d3dContext->RSSetViewports(1, &m_screenViewport);

    // Create a Direct2D target bitmap associated with the
    // swap chain back buffer and set it as the current target.
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = 
        D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            );

    ComPtr<IDXGISurface> dxgiBackBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
        );

    DX::ThrowIfFailed(
        m_d2dContext->CreateBitmapFromDxgiSurface(
            dxgiBackBuffer.Get(),
            &bitmapProperties,
            &m_d2dTargetBitmap
            )
        );

    m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());

    // Grayscale text anti-aliasing is recommended for all Windows Store apps.
    m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}

// This method is called in the event handler for the SizeChanged event.
void DX::DeviceResources::SetLogicalSize(Size logicalSize)
{
    if (m_hwnd == 0)
        return;
    if (m_logicalSize != logicalSize)
    {
        m_logicalSize = logicalSize;
        CreateWindowSizeDependentResources();
    }
}

void DX::DeviceResources::SetWindow(HWND hwnd)
{
    m_hwnd = hwnd;

    RECT rect;
    GetWindowRect(hwnd, &rect);
    m_logicalSize = { (float)(rect.bottom - rect.top), (float)(rect.right - rect.left) };

    CreateWindowSizeDependentResources();
}

// Present the contents of the swap chain to the screen.
void DX::DeviceResources::Present() 
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    DX::ThrowIfFailed(hr);
}

Microsoft::WRL::ComPtr<ID3D11PixelShader> DX::DeviceResources::createPixelShader(
    const std::string &namePrefix) const
{
    ID3D11PixelShader *output;

    std::vector<byte> data;
    bool success = false;
    try
    {
#ifdef _DEBUG
        data = DX::ReadData("x64\\Debug\\" + namePrefix + "PixelShader.cso");
#else
        data = DX::ReadData("x64\\Release\\" + namePrefix + "PixelShader.cso");
#endif // DEBUG
        success = true;
    }
    catch (std::exception &)
    {
    }
    if (!success)
        data = DX::ReadData(namePrefix + "PixelShader.cso");

    DX::ThrowIfFailed(
        m_d3dDevice->CreatePixelShader(
            data.data(),
            data.size(),
            nullptr,
            &output
        )
    );
    std::string shaderName = namePrefix + "PixelShader";
    output->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)shaderName.size(),
        shaderName.c_str());

    return output;
}

Microsoft::WRL::ComPtr<ID3D11VertexShader> DX::DeviceResources::createVertexShader(
    const std::string &namePrefix) const
{
    ID3D11VertexShader *output;

    std::vector<byte> data;
    bool success = false;
    try
    {
#ifdef _DEBUG
        data = DX::ReadData("x64\\Debug\\" + namePrefix + "VertexShader.cso");
#else
        data = DX::ReadData("x64\\Release\\" + namePrefix + "VertexShader.cso");
#endif // DEBUG
        success = true;
    }
    catch (std::exception &)
    {
    }
    if (!success)
        data = DX::ReadData(namePrefix + "VertexShader.cso");

    DX::ThrowIfFailed(
        m_d3dDevice->CreateVertexShader(
            data.data(),
            data.size(),
            nullptr,
            &output
        )
    );
    std::string shaderName = namePrefix + "VertexShader";
    output->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)shaderName.size(),
        shaderName.c_str());

    return output;
}
