#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"
#include "Content\SampleFpsTextRenderer.h"

// Renders Direct2D and 3D content on the screen.
namespace anim
{
    class AnimMain
    {
    public:
        AnimMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        ~AnimMain();
        void CreateWindowSizeDependentResources();
        void Update();
        bool Render();

    private:
        struct RenderTargetTexture
        {
            Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
            D3D11_VIEWPORT viewport;
        };

        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // TODO: Replace with your own content renderers.
        std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
        std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;

        // Rendering loop timer.
        DX::StepTimer m_timer;

        // Post-proccessing shaders
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelCopyShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelBrightnessShader;

        // Scene render target
        RenderTargetTexture m_sceneRenderTarget;

        // Render targets of decreasing sizes used to calculate frame average brightness
        std::vector<RenderTargetTexture> m_averagingRenderTargets;

        // Create texture of given size and bind it as render target and shader resource
        RenderTargetTexture createRenderTargetTexture(const DX::Size &size,
            const std::string &namePrefix) const;

        void copyTexture(const RenderTargetTexture &source,
            const RenderTargetTexture &dest) const;
    };
}