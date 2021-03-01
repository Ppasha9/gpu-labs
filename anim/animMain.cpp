#include "pch.h"
#include "animMain.h"
#include "Common\DirectXHelper.h"

using namespace anim;
using namespace Concurrency;
using namespace DirectX;

Microsoft::WRL::ComPtr<ID3D11Texture2D> AnimMain::createCPUAccessibleTexture(
    const DX::Size &size, const std::string &namePrefix)
{
    ID3D11Texture2D *output;

    CD3D11_TEXTURE2D_DESC textureDesc(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        lround(size.width),
        lround(size.height),
        1, // Only one texture.
        1, // Use a single mipmap level.
        0,
        D3D11_USAGE_STAGING,
        D3D11_CPU_ACCESS_READ
    );

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateTexture2D(
            &textureDesc,
            nullptr,
            &output
        )
    );
    std::string name = namePrefix + "CPUAccTexture";
    output->SetPrivateData(WKPDID_D3DDebugObjectName,
        (UINT)name.size(), name.c_str());

    return output;
}

// Loads and initializes application assets when the application is loaded.
AnimMain::AnimMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
    m_deviceResources(deviceResources)
{
    m_camera = std::make_shared<Camera>();
    m_keyboard = std::make_shared<input::Keyboard>();
    m_mouse = std::make_shared<input::Mouse>();

    m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources, m_camera, m_keyboard));
    m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

    m_vertexShader = deviceResources->createVertexShader("CopyTexture");
    m_copyPixelShader = deviceResources->createPixelShader("CopyTexture");
    m_brightnessPixelShader = deviceResources->createPixelShader("BrightnessCopyTexture");
    m_logPixelShader = deviceResources->createPixelShader("LogCopyTexture");
    m_hdrPixelShader = deviceResources->createPixelShader("HDR");

    // Create post-proccessing constant buffer
    CD3D11_BUFFER_DESC constantBufferDesc(sizeof(PostProcConstBuffer),
        D3D11_BIND_CONSTANT_BUFFER);
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &m_constantBuffer
        )
    );

    // Create 1x1 average brightness texture accessible via CPU
    m_averageBrightnessCPUAccTexture = createCPUAccessibleTexture({ 1, 1 },
        "averageLogBrightness");
}

AnimMain::~AnimMain()
{
}

AnimMain::RenderTargetTexture AnimMain::createRenderTargetTexture(
    const DX::Size &size, const std::string &namePrefix) const
{
    RenderTargetTexture output;

    output.viewport = CD3D11_VIEWPORT(
        0.0f,
        0.0f,
        size.width,
        size.height
    );

    auto device = m_deviceResources->GetD3DDevice();

    // Create texture resource
    output.textureDesc = CD3D11_TEXTURE2D_DESC(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        lround(size.width),
        lround(size.height),
        1, // Only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE
    );

    DX::ThrowIfFailed(
        device->CreateTexture2D(
            &output.textureDesc,
            nullptr,
            &output.texture
        )
    );
    std::string name = namePrefix + "Texture";
    output.texture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(),
        name.c_str());

    // Create post-proccessing texture shader resource view
    DX::ThrowIfFailed(
        device->CreateShaderResourceView(
            output.texture.Get(),
            nullptr,
            &output.shaderResourceView
        )
    );
    name = namePrefix + "ShaderResourceView";
    output.shaderResourceView->SetPrivateData(WKPDID_D3DDebugObjectName,
        (UINT)name.size(), name.c_str());

    // Create post-proccessing texture render target view
    DX::ThrowIfFailed(
        device->CreateRenderTargetView(
            output.texture.Get(),
            nullptr,
            &output.renderTargetView
        )
    );
    name = namePrefix + "RenderTargetView";
    output.renderTargetView->SetPrivateData(WKPDID_D3DDebugObjectName,
        (UINT)name.size(), name.c_str());

    return output;
}

// Updates application state when the window size changes (e.g. device orientation change)
void AnimMain::CreateWindowSizeDependentResources() 
{
    m_sceneRenderer->CreateWindowSizeDependentResources();

    DX::Size size = m_deviceResources->GetLogicalSize();

    // Create scene render target
    m_sceneRenderTarget = createRenderTargetTexture(size, "Scene");
    m_sceneBrightnessRenderTarget = createRenderTargetTexture(size, "SceneBrightness");

    // Create scene brightness texture accessible via CPU
    // to calculate min and max brightness
    m_sceneBrightnessCPUAccTexture = createCPUAccessibleTexture(size, "SceneBrightness");

    // Create averaging render targets
    float minDim = min(size.width, size.height);
    size_t n = 0;
    while (1 << (n + 1) <= minDim)
        n++;
    m_averagingRenderTargets.clear();
    m_averagingRenderTargets.reserve(n + 1);
    for (size_t i = 0; i <= n; i++)
    {
        float dim = (float)(1 << (n - i));
        DX::Size avgSize(dim, dim);
        m_averagingRenderTargets.push_back(createRenderTargetTexture(avgSize,
            std::to_string(avgSize.width) + "x" + std::to_string(avgSize.height)));
    }
}

void AnimMain::InputUpdate(DX::StepTimer const& timer)
{
    while (!m_mouse->EventBufferIsEmpty())
    {
        input::MouseEvent me = m_mouse->ReadEvent();
        if (m_mouse->IsRightDown())
        {
            if (me.GetType() == input::MouseEvent::EventType::RAW_MOVE)
            {
                m_camera->AdjustRotation((float)me.GetPosY() * 0.01f, (float)me.GetPosX() * (-0.01f), 0);
            }
        }
    }

    const float cameraSpeed = 0.02f;

    if (m_keyboard->KeyIsPressed('W'))
    {
        m_camera->AdjustPosition(m_camera->GetForwardVector() * cameraSpeed);
    }
    if (m_keyboard->KeyIsPressed('S'))
    {
        m_camera->AdjustPosition(m_camera->GetBackwardVector() * cameraSpeed);
    }
    if (m_keyboard->KeyIsPressed('A'))
    {
        m_camera->AdjustPosition(m_camera->GetRightVector() * cameraSpeed);
    }
    if (m_keyboard->KeyIsPressed('D'))
    {
        m_camera->AdjustPosition(m_camera->GetLeftVector() * cameraSpeed);
    }
    if (m_keyboard->KeyIsPressed(VK_SPACE))
    {
        m_camera->AdjustPosition(0.0f, cameraSpeed, 0.0f);
    }
    if (m_keyboard->KeyIsPressed('Z'))
    {
        m_camera->AdjustPosition(0.0f, -cameraSpeed, 0.0f);
    }
}

// Updates the application state once per frame.
void AnimMain::Update()
{
    // Update scene objects.
    m_timer.Tick([&]()
    {
        InputUpdate(m_timer);

        // TODO: Replace this with your app's content update functions.
        m_sceneRenderer->Update(m_timer);
        m_fpsTextRenderer->Update(m_timer);
    });
}

void AnimMain::copyTexture(const RenderTargetTexture &source, const RenderTargetTexture &dest) const
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Set destination texture as render target
    context->OMSetRenderTargets(1, dest.renderTargetView.GetAddressOf(), nullptr);
    // Set viewport
    context->RSSetViewports(1, &dest.viewport);
    // Set source render target as shader resource
    context->PSSetShaderResources(0, 1, source.shaderResourceView.GetAddressOf());

    // Render full-screen quad
    context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->PSSetSamplers(0, 1, m_deviceResources->GetSamplerState());
    context->Draw(4, 0);
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool AnimMain::Render() 
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return false;
    }

    auto context = m_deviceResources->GetD3DDeviceContext();

    // Set scene render target
    ID3D11RenderTargetView *const sceneTargets[1] = { m_sceneRenderTarget.renderTargetView.Get() };
    context->OMSetRenderTargets(1, sceneTargets, m_deviceResources->GetDepthStencilView());
    context->RSSetViewports(1, &m_sceneRenderTarget.viewport);

    // Clear back buffer, averaging textures and depth stencil view.
    context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
    context->ClearRenderTargetView(m_sceneRenderTarget.renderTargetView.Get(), DirectX::Colors::CornflowerBlue);
    for (auto &rtt : m_averagingRenderTargets)
        context->ClearRenderTargetView(rtt.renderTargetView.Get(), DirectX::Colors::CornflowerBlue);
    context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Render the 3d scene
    m_sceneRenderer->Render();

    // Render post-proccessing texture to the screen
    auto annotation = m_deviceResources->GetAnnotation();
    annotation->BeginEvent(L"Post-Proccessing");
    annotation->BeginEvent(L"Calculate frame average brightness");
    // Attach copy texture vertex shader
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

    // Calculate scene brightness
    context->PSSetShader(m_brightnessPixelShader.Get(), nullptr, 0);
    copyTexture(m_sceneRenderTarget, m_sceneBrightnessRenderTarget);

    // Calculate scene brightness logarithm
    context->PSSetShader(m_logPixelShader.Get(), nullptr, 0);
    copyTexture(m_sceneBrightnessRenderTarget, m_averagingRenderTargets.front());

    // Continuously copy to smaller textures
    context->PSSetShader(m_copyPixelShader.Get(), nullptr, 0);
    for (size_t i = 1; i < m_averagingRenderTargets.size(); i++)
        copyTexture(m_averagingRenderTargets[i - 1], m_averagingRenderTargets[i]);
    annotation->EndEvent(); // Calculate frame average brightness

    annotation->BeginEvent(L"Render with HDR to screen");
    // Set render target to screen
    ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
    context->OMSetRenderTargets(1, targets, nullptr);
    // Reset the viewport to target the whole screen.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);
    // Attach HDR shader
    context->PSSetShader(m_hdrPixelShader.Get(), nullptr, 0);

    // Calculate adapted average brightness
    context->CopyResource(m_averageBrightnessCPUAccTexture.Get(),
        m_averagingRenderTargets.back().texture.Get());
    D3D11_MAPPED_SUBRESOURCE averageBrightnessAccessor;
    DX::ThrowIfFailed(
        context->Map(
            m_averageBrightnessCPUAccTexture.Get(),
            0,
            D3D11_MAP_READ,
            0,
            &averageBrightnessAccessor
        )
    );
    float averageLogBrightness = *(float *)averageBrightnessAccessor.pData;
    m_adaptedAverageLogBrightness += (averageLogBrightness - m_adaptedAverageLogBrightness) *
        (float)(1 - std::exp(-m_timer.GetElapsedSeconds() / m_adaptationTime));

    // Calculate min and max scene brightness
    //context->CopyResource(m_sceneBrightnessCPUAccTexture.Get(),
    //    m_sceneBrightnessRenderTarget.texture.Get());
    //D3D11_MAPPED_SUBRESOURCE sceneBrightnessAccessor;
    //DX::ThrowIfFailed(
    //    context->Map(
    //        m_sceneBrightnessCPUAccTexture.Get(),
    //        0,
    //        D3D11_MAP_READ,
    //        0,
    //        &sceneBrightnessAccessor
    //    )
    //);
    //float *data = (float *)sceneBrightnessAccessor.pData;
    //float minL = 99999999.0f, maxL = 0;
    float minL = 0, maxL = 1;
    //size_t w = m_sceneBrightnessRenderTarget.textureDesc.Width;
    //size_t h = m_sceneBrightnessRenderTarget.textureDesc.Height;
    //size_t pitch = sceneBrightnessAccessor.RowPitch / sizeof(float);
    //for (size_t i = 0; i < h; i += max(1, h / 2))
    //    for (size_t j = 0; j < w; j += max(1, w / 2))
    //    {
    //        float value = *(data + i * pitch + j * 4);
    //        minL = min(minL, value);
    //        maxL = max(maxL, value);
    //    }

    //Set constant buffer parameter
    m_postProcData.averageLogBrightness = m_adaptedAverageLogBrightness;
    m_postProcData.minBrightness = minL;
    m_postProcData.maxBrightness = maxL;
    context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL,
        &m_postProcData, 0, 0);
    context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    // Set scene texture as shader resource
    context->PSSetShaderResources(0, 1, m_sceneRenderTarget.shaderResourceView.GetAddressOf());
    //context->PSSetShaderResources(0, 1, m_averagingRenderTargets.back().shaderResourceView.GetAddressOf());
    // Render full-screen quad
    context->Draw(4, 0);
    annotation->EndEvent(); // Render with HDR to screen
    annotation->EndEvent(); // Post-proccessing

    // Render GUI
    m_fpsTextRenderer->Render();

    return true;
}

bool AnimMain::IsKeysAutoRepeat() const
{
    return m_keyboard->IsKeysAutoRepeat();
}

void AnimMain::OnKeyPressed(const unsigned char keycode)
{
    m_keyboard->OnKeyPressed(keycode);
}

void AnimMain::OnKeyReleased(const unsigned char keycode)
{
    m_keyboard->OnKeyReleased(keycode);
}

bool AnimMain::IsCharsAutoRepeat() const
{
    return m_keyboard->IsCharsAutoRepeat();
}

void AnimMain::OnChar(const unsigned char ch)
{
    m_keyboard->OnChar(ch);
}

void AnimMain::OnLeftPressed(int x, int y)
{
    m_mouse->OnLeftPressed(x, y);
}

void AnimMain::OnLeftReleased(int x, int y)
{
    m_mouse->OnLeftReleased(x, y);
}

void AnimMain::OnRightPressed(int x, int y)
{
    m_mouse->OnRightPressed(x, y);
}

void AnimMain::OnRightReleased(int x, int y)
{
    m_mouse->OnRightReleased(x, y);
}

void AnimMain::OnMiddlePressed(int x, int y)
{
    m_mouse->OnMiddlePressed(x, y);
}

void AnimMain::OnMiddleReleased(int x, int y)
{
    m_mouse->OnMiddleReleased(x, y);
}

void AnimMain::OnWheelUp(int x, int y)
{
    m_mouse->OnWheelUp(x, y);
}

void AnimMain::OnWheelDown(int x, int y)
{
    m_mouse->OnWheelDown(x, y);
}

void AnimMain::OnMouseMove(int x, int y)
{
    m_mouse->OnMouseMove(x, y);
}

void AnimMain::OnMouseMoveRaw(int x, int y)
{
    m_mouse->OnMouseMoveRaw(x, y);
}

