#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Common\Camera\Camera.h"
#include "Common\Input\Mouse.h"
#include "Common\Input\Keyboard.h"
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

        // Input system methods
        bool IsKeysAutoRepeat() const;
        void OnKeyPressed(const unsigned char keycode);
        void OnKeyReleased(const unsigned char keycode);

        bool IsCharsAutoRepeat() const;
        void OnChar(const unsigned char ch);

        void OnLeftPressed(int x, int y);
        void OnLeftReleased(int x, int y);

        void OnRightPressed(int x, int y);
        void OnRightReleased(int x, int y);

        void OnMiddlePressed(int x, int y);
        void OnMiddleReleased(int x, int y);

        void OnWheelUp(int x, int y);
        void OnWheelDown(int x, int y);

        void OnMouseMove(int x, int y);
        void OnMouseMoveRaw(int x, int y);

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

        // Camera handler
        std::shared_ptr<Camera> m_camera;

        // Input system handlers
        std::shared_ptr<input::Keyboard> m_keyboard;
        std::shared_ptr<input::Mouse> m_mouse;

        // Rendering loop timer.
        DX::StepTimer m_timer;

        // Post-proccessing shaders
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_copyPixelShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_brightnessPixelShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_hdrPixelShader;

        // Scene render target
        RenderTargetTexture m_sceneRenderTarget;

        // Render targets of decreasing sizes used to calculate frame average brightness
        std::vector<RenderTargetTexture> m_averagingRenderTargets;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_averageBrightnessCPUAccTexture;

        // Post-proccessing constant buffer
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
        struct PostProcConstBuffer
        {
            float averageLogBrightness;
            float dummy[3];
        } m_postProcData;

        float m_adaptedAverageLogBrightness = 0;
        float m_adaptationTime = 2;

        D3D11_MAPPED_SUBRESOURCE m_averageBrightnessAccessor;

        // Create texture of given size and bind it as render target and shader resource
        RenderTargetTexture createRenderTargetTexture(const DX::Size &size,
            const std::string &namePrefix) const;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> createCPUAccessibleTexture(
            const DX::Size &size, const std::string &namePrefix);

        void copyTexture(const RenderTargetTexture &source,
            const RenderTargetTexture &dest) const;

        void InputUpdate(DX::StepTimer const& timer);

        void UnbindShaderResource() const;
    };
}