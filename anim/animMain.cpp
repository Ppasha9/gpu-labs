﻿#include "pch.h"

#include "animMain.h"
#include "Common\DirectXHelper.h"

using namespace anim;
using namespace Concurrency;
using namespace DirectX;

Microsoft::WRL::ComPtr<ID3D11Texture2D> AnimMain::createCPUAccessibleTexture(
    const DX::Size &size, const std::string &namePrefix)
{
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

    return m_deviceResources->createTexture2D(textureDesc, namePrefix + "CPUAcc");
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

// Updates application state when the window size changes (e.g. device orientation change)
void AnimMain::CreateWindowSizeDependentResources() 
{
    m_sceneRenderer->CreateWindowSizeDependentResources();

    DX::Size size = m_deviceResources->GetLogicalSize();

    // Create scene render target
    m_sceneRenderTarget = m_deviceResources->createRenderTargetTexture(size, "Scene");

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
        m_averagingRenderTargets.push_back(
            m_deviceResources->createRenderTargetTexture(avgSize,
            std::to_string(lround(avgSize.width)) + "x" + std::to_string(lround(avgSize.height))));
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

    if (m_keyboard->KeyWasReleased('3'))
        isHDR = true;
    if (m_keyboard->KeyWasReleased('4') ||
        m_keyboard->KeyWasReleased('5') ||
        m_keyboard->KeyWasReleased('6'))
        isHDR = false;
}

// Updates the application state once per frame.
void AnimMain::Update()
{
    // Update scene objects.
    m_timer.Tick([&]()
    {
        InputUpdate(m_timer);

        m_sceneRenderer->Update(m_timer);
        m_fpsTextRenderer->Update(m_timer);
    });

    m_keyboard->Update();
}

void AnimMain::copyTexture(const DX::RenderTargetTexture &source,
    const DX::RenderTargetTexture &dest) const
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Set destination texture as render target
    UnbindShaderResource();
    context->OMSetRenderTargets(1, dest.renderTargetView.GetAddressOf(), nullptr);
    // Set viewport
    context->RSSetViewports(1, &dest.viewport);
    // Set source render target as shader resource
    context->PSSetShaderResources(0, 1, source.shaderResourceView.GetAddressOf());

    // Render full-screen quad
    context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->PSSetSamplers(0, 1, m_deviceResources->GetSamplerStateWrap());
    context->Draw(4, 0);
}

void AnimMain::UnbindShaderResource() const
{
    ID3D11ShaderResourceView *const pSRV[1] = { NULL };
    m_deviceResources->GetD3DDeviceContext()->PSSetShaderResources(0, 1, pSRV);
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
    UnbindShaderResource();
    if (isHDR)
        context->OMSetRenderTargets(1, m_sceneRenderTarget.renderTargetView.GetAddressOf(), m_deviceResources->GetDepthStencilView());
    else
    {
        ID3D11RenderTargetView * const rt[] = { m_deviceResources->GetBackBufferRenderTargetView() };
        context->OMSetRenderTargets(1, rt, m_deviceResources->GetDepthStencilView());
    }

    context->RSSetViewports(1, &m_sceneRenderTarget.viewport);

    // Clear back buffer, averaging textures and depth stencil view.
    context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
    context->ClearRenderTargetView(m_sceneRenderTarget.renderTargetView.Get(), DirectX::Colors::CornflowerBlue);
    for (auto &rtt : m_averagingRenderTargets)
        context->ClearRenderTargetView(rtt.renderTargetView.Get(), DirectX::Colors::CornflowerBlue);
    context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Render the 3d scene
    m_sceneRenderer->Render();

    if (isHDR)
    {
        // Render post-proccessing texture to the screen
        auto annotation = m_deviceResources->GetAnnotation();
        annotation->BeginEvent(L"Post-Proccessing");
        annotation->BeginEvent(L"Calculate frame average brightness");
        // Attach copy texture vertex shader
        context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

        // Calculate log of scene brightness
        context->PSSetShader(m_brightnessPixelShader.Get(), nullptr, 0);
        copyTexture(m_sceneRenderTarget, m_averagingRenderTargets.front());

        // Continuously copy to smaller textures
        context->PSSetShader(m_copyPixelShader.Get(), nullptr, 0);
        for (size_t i = 1; i < m_averagingRenderTargets.size(); i++)
            copyTexture(m_averagingRenderTargets[i - 1], m_averagingRenderTargets[i]);
        annotation->EndEvent(); // Calculate frame average brightness

        annotation->BeginEvent(L"Render with HDR to screen");
        // Set render target to screen
        ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
        UnbindShaderResource();
        context->OMSetRenderTargets(1, targets, nullptr);
        // Reset the viewport to target the whole screen.
        auto viewport = m_deviceResources->GetScreenViewport();
        context->RSSetViewports(1, &viewport);
        // Attach HDR shader
        context->PSSetShader(m_hdrPixelShader.Get(), nullptr, 0);

        // Calculate adapted average brightness
        context->CopyResource(m_averageBrightnessCPUAccTexture.Get(),
            m_averagingRenderTargets.back().texture.Get());
        DX::ThrowIfFailed(
            context->Map(
                m_averageBrightnessCPUAccTexture.Get(),
                0,
                D3D11_MAP_READ,
                0,
                &m_averageBrightnessAccessor
            )
        );
        float averageLogBrightness = *(float *)m_averageBrightnessAccessor.pData;
        m_adaptedAverageLogBrightness += (averageLogBrightness - m_adaptedAverageLogBrightness) *
            (float)(1 - std::exp(-m_timer.GetElapsedSeconds() / m_adaptationTime));
        context->Unmap(m_averageBrightnessCPUAccTexture.Get(), 0);

        //Set constant buffer parameter
        m_postProcData.averageLogBrightness = m_adaptedAverageLogBrightness;
        context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL,
            &m_postProcData, 0, 0);
        context->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        // Set scene texture as shader resource
        context->PSSetShaderResources(0, 1, m_sceneRenderTarget.shaderResourceView.GetAddressOf());
        // Render full-screen quad
        context->Draw(4, 0);
        annotation->EndEvent(); // Render with HDR to screen
        annotation->EndEvent(); // Post-proccessing
    }

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
