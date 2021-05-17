#pragma once

#include "../tiny_gltf.h"
#include "../DeviceResources.h"
#include "../../Content/ShaderStructures.h"


class GeomModel
{
public:
    GeomModel  (const std::string& modelPath, const std::string& modelName);
    ~GeomModel () = default;

    HRESULT CreateDeviceDependentResources (const std::shared_ptr<DX::DeviceResources>& deviceResources);

    void Render (
        std::shared_ptr<DX::DeviceResources>& deviceResources,
        anim::ModelViewProjectionConstantBuffer& cameraData, ID3D11Buffer* cameraConstantBuffer,
        anim::LightConstantBuffer& lightData, ID3D11Buffer* lightConstantBuffer,
        anim::GeneralConstantBuffer& generalData, ID3D11Buffer* generalConstantBuffer,
        ID3D11ShaderResourceView* irradianceMapSRV, ID3D11ShaderResourceView* prefilteredColorMapSRV, ID3D11ShaderResourceView* preintegratedBRDFSRV,
        DirectX::XMVECTOR cameraDir);

private:
    struct GeomPrimitiveMaterial
    {
        std::string name;

        bool blend;

        Microsoft::WRL::ComPtr<ID3D11BlendState>      pBlendState;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> pRasterizerState;
        Microsoft::WRL::ComPtr<ID3D11InputLayout>     pInputLayout;
        Microsoft::WRL::ComPtr<ID3D11VertexShader>    pVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>     pPixelShader;

        int baseColorTexture;
        int metallicRoughnessTexture;
        int normalTexture;

        std::vector<D3D_SHADER_MACRO> defines;

        anim::GeomModelMaterialConstantBuffer materialBufferData;
    };

    struct GeomAttribute
    {
        UINT byteStride;
        Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;
    };

    struct GeomPrimitive
    {
        std::vector<GeomAttribute> attributes;
        UINT vertexCount;

        Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;
        UINT indexCount;

        DirectX::XMVECTOR max;
        DirectX::XMVECTOR min;

        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
        DXGI_FORMAT indexFormat;

        UINT material;
        UINT matrix;
    };

    HRESULT CreateTextures     (const std::shared_ptr<DX::DeviceResources>& deviceResources, tinygltf::Model& model);
    HRESULT CreateSamplerState (const std::shared_ptr<DX::DeviceResources>& deviceResources, tinygltf::Model& model);
    HRESULT CreateMaterials    (const std::shared_ptr<DX::DeviceResources>& deviceResources, tinygltf::Model& model);
    HRESULT CreateShaders      (const std::shared_ptr<DX::DeviceResources>& deviceResources, D3D_SHADER_MACRO* defines, ID3D11VertexShader** vs, ID3D11PixelShader** ps, ID3D11InputLayout** il);
    HRESULT CreatePrimitives   (const std::shared_ptr<DX::DeviceResources>& deviceResources, tinygltf::Model& model);
    HRESULT ProcessNode        (const std::shared_ptr<DX::DeviceResources>& deviceResources, tinygltf::Model& model, tinygltf::Node& node, DirectX::XMMATRIX worldMatrix);
    HRESULT CreatePrimitive    (const std::shared_ptr<DX::DeviceResources>& deviceResources, tinygltf::Model& model, tinygltf::Primitive& gltfPrimitive, size_t matrixIndex);

    void RenderPrimitive(
        GeomPrimitive& primitive,
        std::shared_ptr<DX::DeviceResources>& deviceResources,
        anim::ModelViewProjectionConstantBuffer& cameraData, ID3D11Buffer* cameraConstantBuffer,
        anim::LightConstantBuffer& lightData, ID3D11Buffer* lightConstantBuffer,
        anim::GeneralConstantBuffer& generalData, ID3D11Buffer* generalConstantBuffer,
        ID3D11ShaderResourceView* irradianceMapSRV, ID3D11ShaderResourceView* prefilteredColorMapSRV, ID3D11ShaderResourceView* preintegratedBRDFSRV);

    std::string m_modelPath;
    std::string m_modelName;

    std::vector<Microsoft::WRL::ComPtr<ID3D11Texture2D>>          m_pTextures;
    std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_pShaderResourceViews;

    Microsoft::WRL::ComPtr<ID3D11Buffer>                          m_geomMaterialMaterialConstantBuffer;

    Microsoft::WRL::ComPtr<ID3D11SamplerState>                    m_pSamplerState;

    std::vector<GeomPrimitiveMaterial>                            m_materials;

    std::vector<DirectX::XMMATRIX>                                m_worldMatricies;

    std::vector<GeomPrimitive>                                    m_primitives;
    std::vector<GeomPrimitive>                                    m_transparentPrimitives;
};