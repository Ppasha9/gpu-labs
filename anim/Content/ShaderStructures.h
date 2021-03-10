#pragma once

namespace anim
{
    // Constant buffer used to send MVP matrices to the vertex shader.
    struct ModelViewProjectionConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    };

    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionColorNormal
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT3 color;
        DirectX::XMFLOAT3 normal;
    };

    // Constant buffer used to send light positions, colors and strengths.
    struct LightConstantBuffer
    {
        DirectX::XMFLOAT4 lightColor1;
        DirectX::XMFLOAT4 lightColor2;
        DirectX::XMFLOAT4 lightColor3;
    };

    struct MaterialConstantBuffer
    {
        DirectX::XMFLOAT3 albedo;
        float roughness;
        float metalness;
        float dummy[3];
    };

    struct GeneralConstantBuffer
    {
        DirectX::XMFLOAT3 cameraPos;
        float time;
    };
}