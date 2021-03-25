#pragma once

#include "..\Common\Input\Keyboard.h"
#include "..\Common\Camera\Camera.h"
#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"
#include "ShaderStructures.h"

namespace anim
{
    // This sample renderer instantiates a basic rendering pipeline.
    class Sample3DSceneRenderer
    {
    public:
        Sample3DSceneRenderer(
            const std::shared_ptr<DX::DeviceResources>& deviceResources,
            const std::shared_ptr<Camera>& camera,
            const std::shared_ptr<input::Keyboard>& keyboard);
        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Render();

    private:
        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // Cached pointer to camera handler
        std::shared_ptr<Camera> m_camera;

        // Cached pointer to keyboard handler
        std::shared_ptr<input::Keyboard> m_keyboard;

        // Direct3D resources for cube geometry.
        Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_skySphereVertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_indexBuffer;

        Microsoft::WRL::ComPtr<ID3D11Texture2D>    m_skyCubeMap;

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_unlitVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_unlitPixelShader;

        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_skySpherePixelShader;

        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_normDistrPixelShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_geomPixelShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_fresnelPixelShader;

        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_constantBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_lightConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_materialConstantBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_generalConstantBuffer;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_loadedSkyShaderResourceView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_skyCubeMapShaderResourceView;

        ModelViewProjectionConstantBuffer    m_constantBufferData;
        MaterialConstantBuffer               m_materialConstantBufferData;
        GeneralConstantBuffer                m_generalConstantBufferData;

        size_t                               m_indexCount;

        enum struct PBRShaderMode
        {
            REGULAR,
            NORMAL_DISTRIBUTION,
            GEOMETRY,
            FRESNEL
        } m_shaderMode;

        // Lights information
        LightConstantBuffer                  m_lightConstantBufferData;


        // Constant colors
        const DirectX::XMFLOAT3 LIGHT_COLOR_1 = DirectX::XMFLOAT3(1.0f, 1.0f, 0.8f);
        const DirectX::XMFLOAT3 LIGHT_COLOR_2 = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
        const DirectX::XMFLOAT3 LIGHT_COLOR_3 = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);

        // Variable strength
        float m_strengths[3] = { 0.0f, 0.0f, 0.0f };

        void CycleLight(int lightId);

        void SetMaterial(MaterialConstantBuffer material);

        void renderCubemapTexture();
    };
}

