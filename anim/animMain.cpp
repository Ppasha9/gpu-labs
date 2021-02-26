#include "pch.h"
#include "animMain.h"
#include "Common\DirectXHelper.h"

using namespace anim;
using namespace Concurrency;

// Loads and initializes application assets when the application is loaded.
AnimMain::AnimMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
    m_deviceResources(deviceResources)
{
    // TODO: Replace this with your app's content initialization.
    m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));

    m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */

    // Load and compile post-proccessing shaders
    std::vector<byte> vsData, psData;

    bool success = false;
    try
    {
#ifdef _DEBUG
        vsData = DX::ReadData("x64\\Debug\\PostProcVertexShader.cso");
        psData = DX::ReadData("x64\\Debug\\PostProcPixelShader.cso");
#else
        vsData = DX::ReadData("x64\\Release\\PostProcVertexShader.cso");
        psData = DX::ReadData("x64\\Release\\PostProcPixelShader.cso");
#endif // DEBUG
        success = true;
    }
    catch (std::exception &)
    {
    }
    if (!success)
    {
        vsData = DX::ReadData("PostProcVertexShader.cso");
        psData = DX::ReadData("PostProcPixelShader.cso");
    }

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateVertexShader(
            vsData.data(),
            vsData.size(),
            nullptr,
            &m_vertexShader
        )
    );
    std::string shaderName = "PostProcVertexShader";
    m_vertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)shaderName.size(),
        shaderName.c_str());

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreatePixelShader(
            psData.data(),
            psData.size(),
            nullptr,
            &m_pixelShader
        )
    );
    shaderName = "PostProcPixelShader";
    m_pixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)shaderName.size(),
        shaderName.c_str());
}

AnimMain::~AnimMain()
{
}

// Updates application state when the window size changes (e.g. device orientation change)
void AnimMain::CreateWindowSizeDependentResources() 
{
    // TODO: Replace this with the size-dependent initialization of your app's content.
    m_sceneRenderer->CreateWindowSizeDependentResources();
}

// Updates the application state once per frame.

void AnimMain::Update()
{
    // Update scene objects.
    m_timer.Tick([&]()
    {
        // TODO: Replace this with your app's content update functions.
        m_sceneRenderer->Update(m_timer);
        m_fpsTextRenderer->Update(m_timer);
    });
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

    // Reset the viewport to target the whole screen.
    auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    // Reset render targets to the screen.
    ID3D11RenderTargetView *const ppTargets[1] = { m_deviceResources->GetPostProccessingRenderTargetView() };
    context->OMSetRenderTargets(1, ppTargets, m_deviceResources->GetDepthStencilView());

    // Clear the back buffer and depth stencil view.
    context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
    context->ClearRenderTargetView(m_deviceResources->GetPostProccessingRenderTargetView(), DirectX::Colors::CornflowerBlue);
    context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Render the 3d scene
    m_sceneRenderer->Render();

    // Render post-proccessing texture to the screen
    auto annotation = m_deviceResources->GetAnnotation();
    annotation->BeginEvent(L"Post-Proccessing");

    ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
    context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

    // Attach post-processing shaders
    context->VSSetShader(
        m_vertexShader.Get(),
        nullptr,
        0
    );
    context->PSSetShader(
        m_pixelShader.Get(),
        nullptr,
        0
    );
    // Render full-screen quad
    context->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->PSSetSamplers(0, 1, m_deviceResources->GetSamplerState());
    context->PSSetShaderResources(0, 1, m_deviceResources->GetPostProccessingShaderResourceView());
    context->Draw(4, 0);

    annotation->EndEvent();

    // Render GUI
    m_fpsTextRenderer->Render();

    return true;
}
