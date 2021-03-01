#pragma once

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
        Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<Camera>& camera);
        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Render();

    private:
        void Rotate(float radians);

        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // Cached pointer to camera handler
        std::shared_ptr<Camera> m_camera;

        // Direct3D resources for cube geometry.
        Microsoft::WRL::ComPtr<ID3D11InputLayout>  m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_indexBuffer;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11Buffer>       m_constantBuffer;

        // System resources for cube geometry.
        ModelViewProjectionConstantBuffer    m_constantBufferData;
        size_t    m_indexCount;

        // Variables used with the rendering loop.
        float   m_degreesPerSecond;
        bool    m_tracking;
    };
}

