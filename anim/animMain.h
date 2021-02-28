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
            CD3D11_TEXTURE2D_DESC textureDesc;
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
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_copyPixelShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_brightnessPixelShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_logPixelShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_hdrPixelShader;

        // Scene render target
        RenderTargetTexture m_sceneRenderTarget;
        RenderTargetTexture m_sceneBrightnessRenderTarget;

        // Render targets of decreasing sizes used to calculate frame average brightness
        std::vector<RenderTargetTexture> m_averagingRenderTargets;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_averageBrightnessCPUAccTexture;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_sceneBrightnessCPUAccTexture;

        // Post-proccessing constant buffer
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
        struct PostProcConstBuffer
        {
            float averageLogBrightness;
            float minBrightness;
            float maxBrightness;
            float dummy;
        } m_postProcData;

        float m_adaptedAverageLogBrightness = 0;
        float m_adaptationTime = 2;

        // Create texture of given size and bind it as render target and shader resource
        RenderTargetTexture createRenderTargetTexture(const DX::Size &size,
            const std::string &namePrefix) const;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> createCPUAccessibleTexture(
            const DX::Size &size, const std::string &namePrefix);

        void copyTexture(const RenderTargetTexture &source,
            const RenderTargetTexture &dest) const;
    };
}