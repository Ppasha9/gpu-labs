//#include "../../pch.h"
#include "pch.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include "GeomModel.h"

#include "..\Utils\Utils.h"

using namespace tinygltf;
using namespace Microsoft::WRL;


static const std::string s_modelsRootPath("..\\..\\Models\\");


GeomModel::GeomModel (const std::string& modelPath, const std::string& modelName)
    : m_modelPath(s_modelsRootPath + modelPath)
    , m_modelName(modelName)
{
}


HRESULT GeomModel::CreateDeviceDependentResources (const std::shared_ptr<DX::DeviceResources>& deviceResources)
{
    TinyGLTF modelLoader;
    Model modelStructure;

    bool ret = modelLoader.LoadASCIIFromFile(&modelStructure, nullptr, nullptr, m_modelPath);
    if (!ret) {
        return E_FAIL;
    }

    HRESULT hr = CreateTextures(deviceResources, modelStructure);
    if (FAILED(hr)) {
        return hr;
    }

    hr = CreateSamplerState(deviceResources, modelStructure);
    if (FAILED(hr)) {
        return hr;
    }

    CD3D11_BUFFER_DESC materialConstantBufferDsc(sizeof(anim::GeomModelMaterialConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    hr = deviceResources->GetD3DDevice()->CreateBuffer(&materialConstantBufferDsc, nullptr, &m_geomMaterialMaterialConstantBuffer);
    if (FAILED(hr)) {
        return hr;
    }

    hr = CreateMaterials(deviceResources, modelStructure);
    if (FAILED(hr)) {
        return hr;
    }

    hr = CreatePrimitives(deviceResources, modelStructure);
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}


HRESULT GeomModel::CreateTextures (const std::shared_ptr<DX::DeviceResources>& deviceResources, Model& model)
{
    for (Image& img : model.images) {
        ComPtr<ID3D11Texture2D> texture;
        CD3D11_TEXTURE2D_DESC textDsc(DXGI_FORMAT_R8G8B8A8_UNORM, img.width, img.height, 1, 1, D3D11_BIND_SHADER_RESOURCE);
        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = img.image.data();
        data.SysMemPitch = 4 * img.width;

        HRESULT hr = deviceResources->GetD3DDevice()->CreateTexture2D(
            &textDsc,
            &data,
            &texture
        );
        if (FAILED(hr)) {
            return hr;
        }

        m_pTextures.push_back(texture);

        ComPtr<ID3D11ShaderResourceView> srv;
        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDsc(D3D11_SRV_DIMENSION_TEXTURE2D, textDsc.Format);
        hr = deviceResources->GetD3DDevice()->CreateShaderResourceView(texture.Get(), &srvDsc, &srv);
        if (FAILED(hr)) {
            return hr;
        }

        m_pShaderResourceViews.push_back(srv);
    }

    return S_OK;
}


static D3D11_TEXTURE_ADDRESS_MODE s_getTextureAddressMode (int wrap)
{
    switch (wrap)
    {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return D3D11_TEXTURE_ADDRESS_MIRROR;
    default:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    }
}


HRESULT GeomModel::CreateSamplerState (const std::shared_ptr<DX::DeviceResources>& deviceResources, Model& model)
{
    Sampler& gltfSampler = model.samplers[0];

    D3D11_SAMPLER_DESC samplerDsc;
    ZeroMemory(&samplerDsc, sizeof(samplerDsc));

    switch (gltfSampler.minFilter) {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            samplerDsc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        else
            samplerDsc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            samplerDsc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else
            samplerDsc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            samplerDsc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        else
            samplerDsc.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            samplerDsc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
        else
            samplerDsc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            samplerDsc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
        else
            samplerDsc.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
        break;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        if (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
            samplerDsc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
        else
            samplerDsc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        break;
    default:
        samplerDsc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        break;
    }

    samplerDsc.AddressU = s_getTextureAddressMode(gltfSampler.wrapS);
    samplerDsc.AddressV = s_getTextureAddressMode(gltfSampler.wrapT);
    samplerDsc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDsc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDsc.MinLOD = 0;
    samplerDsc.MaxLOD = D3D11_FLOAT32_MAX;

    return deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDsc, &m_pSamplerState);
}


HRESULT GeomModel::CreateMaterials (const std::shared_ptr<DX::DeviceResources>& deviceResources, Model& model)
{
    for (Material& gltfMaterial : model.materials) {
        GeomPrimitiveMaterial material = {};

        material.name = gltfMaterial.name;
        material.blend = false;

        D3D11_BLEND_DESC blendDsc;
        ZeroMemory(&blendDsc, sizeof(blendDsc));
        if (gltfMaterial.alphaMode == "BLEND") {
            material.blend = true;
            blendDsc.RenderTarget[0].BlendEnable = true;
            blendDsc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            blendDsc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            blendDsc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDsc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
            blendDsc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            blendDsc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDsc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            HRESULT hr = deviceResources->GetD3DDevice()->CreateBlendState(&blendDsc, &material.pBlendState);
            if (FAILED(hr)) {
                return hr;
            }
        }

        D3D11_RASTERIZER_DESC rasterizerDsc;
        ZeroMemory(&rasterizerDsc, sizeof(rasterizerDsc));
        rasterizerDsc.FillMode = D3D11_FILL_SOLID;
        if (gltfMaterial.doubleSided) {
            rasterizerDsc.CullMode = D3D11_CULL_NONE;
        } else {
            rasterizerDsc.CullMode = D3D11_CULL_BACK;
        }
        rasterizerDsc.FrontCounterClockwise = true;
        rasterizerDsc.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
        rasterizerDsc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerDsc.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        HRESULT hr = deviceResources->GetD3DDevice()->CreateRasterizerState(&rasterizerDsc, &material.pRasterizerState);
        if (FAILED(hr)) {
            return hr;
        }

        float albedo[4];
        for (short i = 0; i < 4; ++i) {
            albedo[i] = static_cast<float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor[i]);
        }

        material.materialBufferData.albedo = DirectX::XMFLOAT4(albedo);
        material.materialBufferData.roughness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);
        material.materialBufferData.metalness = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);

        std::vector<D3D_SHADER_MACRO> defines;

        material.baseColorTexture = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
        if (material.baseColorTexture >= 0) {
            defines.push_back({"HAS_BASE_COLOR_TEXTURE", "True"});
        }

        material.metallicRoughnessTexture = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (material.metallicRoughnessTexture >= 0) {
            defines.push_back({ "HAS_METALLIC_ROUGHNESS_TEXTURE", "True" });
        }

        material.normalTexture = gltfMaterial.normalTexture.index;
        if (material.normalTexture >= 0) {
            defines.push_back({ "HAS_NORMAL_TEXTURE", "True" });
        }

        if (gltfMaterial.occlusionTexture.index >= 0) {
            defines.push_back({"HAS_OCCLUSION_TEXTURE", "True"});
        }

        defines.push_back({ nullptr, nullptr });

        hr = CreateShaders(deviceResources, defines.data(), &material.pVertexShader, &material.pPixelShader, &material.pInputLayout);
        if (FAILED(hr)) {
            return hr;
        }

        m_materials.push_back(material);
    }

    return S_OK;
}


HRESULT GeomModel::CreateShaders (const std::shared_ptr<DX::DeviceResources>& deviceResources, D3D_SHADER_MACRO* defines, ID3D11VertexShader** vs, ID3D11PixelShader** ps, ID3D11InputLayout** il)
{
    // ---------- Create vertex shader ---------- //
    ComPtr<ID3DBlob> blob;
    HRESULT hr;

    hr = CompileShaderFromFile(L"..\\..\\Content\\GeomModelShaders.fx", "vs_main", "vs_5_0", &blob, defines);
    if (FAILED(hr)) {
        return hr;
    }

    hr = deviceResources->GetD3DDevice()->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, vs);
    if (FAILED(hr)) {
        return hr;
    }

    // ---------- Create input layout ---------- //
    static const D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD_", 0, DXGI_FORMAT_R32G32_FLOAT, 2, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = deviceResources->GetD3DDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), blob->GetBufferPointer(), blob->GetBufferSize(), il);
    if (FAILED(hr)) {
        return hr;
    }

    // ---------- Create pixel shader ---------- //
    hr = CompileShaderFromFile(L"..\\..\\Content\\GeomModelShaders.fx", "ps_main", "ps_5_0", &blob, defines);
    if (FAILED(hr)) {
        return hr;
    }

    hr = deviceResources->GetD3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, ps);
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}


HRESULT GeomModel::CreatePrimitives (const std::shared_ptr<DX::DeviceResources>& deviceResources, Model& model)
{
    Scene& gltfScene = model.scenes[model.defaultScene];

    m_worldMatricies.push_back(DirectX::XMMatrixIdentity());

    for (int nodeIndex : gltfScene.nodes) {
        HRESULT hr = ProcessNode(deviceResources, model, model.nodes[nodeIndex], m_worldMatricies[0]);
        if (FAILED(hr)) {
            return hr;
        }
    }

    return S_OK;
}


static DirectX::XMMATRIX s_getMatrixFromNode(Node& gltfNode)
{
    if (gltfNode.matrix.empty()) {
        DirectX::XMMATRIX rotation;
        if (gltfNode.rotation.empty()) {
            rotation = DirectX::XMMatrixIdentity();
        } else {
            float v[4] = {};
            for (int i = 0; i < gltfNode.rotation.size(); i++) {
                v[i] = static_cast<float>(gltfNode.rotation[i]);
            }
            DirectX::XMFLOAT4 vector(v);
            rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&vector));
        }

        return rotation;
    }

    float flat[16] = {};
    for (int i = 0; i < gltfNode.matrix.size(); i++) {
        flat[i] = static_cast<float>(gltfNode.matrix[i]);
    }
    DirectX::XMFLOAT4X4 matrix(flat);
    return DirectX::XMLoadFloat4x4(&matrix);
}


HRESULT GeomModel::ProcessNode (const std::shared_ptr<DX::DeviceResources>& deviceResources, Model& model, Node& node, DirectX::XMMATRIX worldMatrix)
{
    if (node.mesh >= 0) {
        Mesh& gltfMesh = model.meshes[node.mesh];
        size_t matrixIndex = m_worldMatricies.size() - 1;

        for (Primitive& gltfPrimitive : gltfMesh.primitives) {
            HRESULT hr = CreatePrimitive(deviceResources, model, gltfPrimitive, matrixIndex);
            if (FAILED(hr)) {
                return hr;
            }
        }
    }

    if (node.children.size() > 0) {
        worldMatrix = DirectX::XMMatrixMultiplyTranspose(DirectX::XMMatrixTranspose(worldMatrix), DirectX::XMMatrixTranspose(s_getMatrixFromNode(node)));
        m_worldMatricies.push_back(worldMatrix);
        for (int childNodeIdx : node.children) {
            HRESULT hr = ProcessNode(deviceResources, model, model.nodes[childNodeIdx], worldMatrix);
            if (FAILED(hr)) {
                return hr;
            }
        }
    }

    return S_OK;
}


HRESULT GeomModel::CreatePrimitive (const std::shared_ptr<DX::DeviceResources>& deviceResources, Model& model, Primitive& gltfPrimitive, size_t matrixIndex)
{
    GeomPrimitive primitive = {};

    for (std::pair<const std::string, int>& prAttribute : gltfPrimitive.attributes) {
        if (prAttribute.first == "TEXCOORD_1") {
            continue;
        }

        Accessor& gltfAccessor = model.accessors[prAttribute.second];
        BufferView& gltfBufferView = model.bufferViews[gltfAccessor.bufferView];
        Buffer& gltfBuffer = model.buffers[gltfBufferView.buffer];

        GeomAttribute attribute = {};
        attribute.byteStride = static_cast<UINT>(gltfAccessor.ByteStride(gltfBufferView));

        CD3D11_BUFFER_DESC vertexBufferDsc(attribute.byteStride * static_cast<UINT>(gltfAccessor.count), D3D11_BIND_VERTEX_BUFFER);
        vertexBufferDsc.StructureByteStride = attribute.byteStride;

        D3D11_SUBRESOURCE_DATA vertexBufferData;
        ZeroMemory(&vertexBufferData, sizeof(D3D11_SUBRESOURCE_DATA));
        vertexBufferData.pSysMem = &gltfBuffer.data[gltfBufferView.byteOffset + gltfAccessor.byteOffset];

        HRESULT hr = deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDsc, &vertexBufferData, &attribute.pVertexBuffer);
        if (FAILED(hr)) {
            return hr;
        }

        primitive.attributes.push_back(attribute);

        if (prAttribute.first == "POSITION") {
            primitive.vertexCount = static_cast<UINT>(gltfAccessor.count);

            DirectX::XMFLOAT3 maxPosition(static_cast<float>(gltfAccessor.maxValues[0]), static_cast<float>(gltfAccessor.maxValues[1]), static_cast<float>(gltfAccessor.maxValues[2]));
            DirectX::XMFLOAT3 minPosition(static_cast<float>(gltfAccessor.minValues[0]), static_cast<float>(gltfAccessor.minValues[1]), static_cast<float>(gltfAccessor.minValues[2]));

            primitive.max = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&maxPosition), m_worldMatricies[primitive.matrix]);
            primitive.min = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&minPosition), m_worldMatricies[primitive.matrix]);
        }
    }

    switch (gltfPrimitive.mode) {
    case TINYGLTF_MODE_POINTS:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        break;
    case TINYGLTF_MODE_LINE:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        break;
    case TINYGLTF_MODE_LINE_STRIP:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        break;
    case TINYGLTF_MODE_TRIANGLES:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        break;
    case TINYGLTF_MODE_TRIANGLE_STRIP:
        primitive.primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        break;
    }

    Accessor& gltfAccessor = model.accessors[gltfPrimitive.indices];
    BufferView& gltfBufferView = model.bufferViews[gltfAccessor.bufferView];
    Buffer& gltfBuffer = model.buffers[gltfBufferView.buffer];

    primitive.indexCount = static_cast<UINT>(gltfAccessor.count);
    UINT stride = 2;
    switch (gltfAccessor.componentType) {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        primitive.indexFormat = DXGI_FORMAT_R8_UINT;
        stride = 1;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        primitive.indexFormat = DXGI_FORMAT_R16_UINT;
        stride = 2;
        break;
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        primitive.indexFormat = DXGI_FORMAT_R32_UINT;
        stride = 4;
        break;
    }

    CD3D11_BUFFER_DESC indexBufferDsc(stride * primitive.indexCount, D3D11_BIND_INDEX_BUFFER);
    D3D11_SUBRESOURCE_DATA indexBufferData;
    ZeroMemory(&indexBufferData, sizeof(D3D11_SUBRESOURCE_DATA));
    indexBufferData.pSysMem = &gltfBuffer.data[gltfBufferView.byteOffset + gltfAccessor.byteOffset];

    HRESULT hr = deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDsc, &indexBufferData, &primitive.pIndexBuffer);
    if (FAILED(hr)) {
        return hr;
    }

    primitive.material = gltfPrimitive.material;
    primitive.matrix = static_cast<UINT>(matrixIndex);

    if (m_materials[primitive.material].blend) {
        m_transparentPrimitives.push_back(primitive);
    } else {
        m_primitives.push_back(primitive);
    }

    return S_OK;
}


void GeomModel::Render (
    std::shared_ptr<DX::DeviceResources>& deviceResources,
    anim::ModelViewProjectionConstantBuffer& cameraData, ID3D11Buffer* cameraConstantBuffer,
    anim::LightConstantBuffer& lightData, ID3D11Buffer* lightConstantBuffer,
    anim::GeneralConstantBuffer& generalData, ID3D11Buffer* generalConstantBuffer,
    ID3D11ShaderResourceView* irradianceMapSRV, ID3D11ShaderResourceView* prefilteredColorMapSRV, ID3D11ShaderResourceView* preintegratedBRDFSRV
)
{
    ComPtr<ID3DUserDefinedAnnotation> annotation = deviceResources->GetAnnotation();
    annotation->BeginEvent(L"Rendering model's non-transparent primitives");

    ComPtr<ID3D11DeviceContext> context = deviceResources->GetD3DDeviceContext();
    context->VSSetConstantBuffers(0, 1, &cameraConstantBuffer);
    context->PSSetConstantBuffers(1, 1, &lightConstantBuffer);
    context->PSSetConstantBuffers(2, 1, &generalConstantBuffer);
    context->PSSetConstantBuffers(3, 1, m_geomMaterialMaterialConstantBuffer.GetAddressOf());
    context->PSSetSamplers(2, 1, m_pSamplerState.GetAddressOf());

    DirectX::XMStoreFloat4x4(&cameraData.model, DirectX::XMMatrixIdentity());
    for (GeomPrimitive& primitive : m_primitives) {
        RenderPrimitive(primitive, deviceResources, cameraData, cameraConstantBuffer, lightData, lightConstantBuffer, generalData, generalConstantBuffer, irradianceMapSRV, prefilteredColorMapSRV, preintegratedBRDFSRV);
    }

    annotation->EndEvent(); // Rendering model's non-transparent primitives
}


static bool CompareDistancePairs(const std::pair<float, size_t>& p1, const std::pair<float, size_t>& p2)
{
    return p1.first < p2.first;
}


void GeomModel::RenderTransparent (
    std::shared_ptr<DX::DeviceResources>& deviceResources,
    anim::ModelViewProjectionConstantBuffer& cameraData, ID3D11Buffer* cameraConstantBuffer,
    anim::LightConstantBuffer& lightData, ID3D11Buffer* lightConstantBuffer,
    anim::GeneralConstantBuffer& generalData, ID3D11Buffer* generalConstantBuffer,
    ID3D11ShaderResourceView* irradianceMapSRV, ID3D11ShaderResourceView* prefilteredColorMapSRV, ID3D11ShaderResourceView* preintegratedBRDFSRV,
    DirectX::XMVECTOR cameraDir
)
{
    ComPtr<ID3DUserDefinedAnnotation> annotation = deviceResources->GetAnnotation();
    annotation->BeginEvent(L"Rendering model's transparent primitives");

    ComPtr<ID3D11DeviceContext> context = deviceResources->GetD3DDeviceContext();
    context->VSSetConstantBuffers(0, 1, &cameraConstantBuffer);
    context->PSSetConstantBuffers(1, 1, &lightConstantBuffer);
    context->PSSetConstantBuffers(2, 1, &generalConstantBuffer);
    context->PSSetConstantBuffers(3, 1, m_geomMaterialMaterialConstantBuffer.GetAddressOf());
    context->PSSetSamplers(2, 1, m_pSamplerState.GetAddressOf());

    DirectX::XMStoreFloat4x4(&cameraData.model, DirectX::XMMatrixIdentity());

    std::vector<std::pair<float, size_t>> distances;
    float distance;
    DirectX::XMVECTOR center;
    DirectX::XMFLOAT4 cameraPosFloat4 = DirectX::XMFLOAT4(generalData.cameraPos.x, generalData.cameraPos.y, generalData.cameraPos.z, 1.0f);
    DirectX::XMVECTOR cameraPos = DirectX::XMLoadFloat4(&cameraPosFloat4);
    for (size_t i = 0; i < m_transparentPrimitives.size(); ++i) {
        center = DirectX::XMVectorDivide(DirectX::XMVectorAdd(m_transparentPrimitives[i].max, m_transparentPrimitives[i].min), DirectX::XMVectorReplicate(2));
        distance = DirectX::XMVector3Dot(DirectX::XMVectorSubtract(center, cameraPos), cameraDir).m128_f32[0];
        distances.push_back(std::pair<float, size_t>(distance, i));
    }

    std::sort(distances.begin(), distances.end(), CompareDistancePairs);

    for (auto iter = distances.rbegin(); iter != distances.rend(); ++iter) {
        RenderPrimitive(m_transparentPrimitives[iter->second], deviceResources, cameraData, cameraConstantBuffer, lightData, lightConstantBuffer, generalData, generalConstantBuffer, irradianceMapSRV, prefilteredColorMapSRV, preintegratedBRDFSRV);
    }

    annotation->EndEvent(); // Rendering model's transparent primitives
}


void GeomModel::RenderPrimitive (
    GeomPrimitive& primitive,
    std::shared_ptr<DX::DeviceResources>& deviceResources,
    anim::ModelViewProjectionConstantBuffer& cameraData, ID3D11Buffer* cameraConstantBuffer,
    anim::LightConstantBuffer& lightData, ID3D11Buffer* lightConstantBuffer,
    anim::GeneralConstantBuffer& generalData, ID3D11Buffer* generalConstantBuffer,
    ID3D11ShaderResourceView* irradianceMapSRV, ID3D11ShaderResourceView* prefilteredColorMapSRV, ID3D11ShaderResourceView* preintegratedBRDFSRV
)
{
    ComPtr<ID3D11DeviceContext> context = deviceResources->GetD3DDeviceContext();

    std::vector<ID3D11Buffer*> combined;
    std::vector<UINT> offset;
    std::vector<UINT> stride;

    size_t attributesCount = primitive.attributes.size();
    combined.resize(attributesCount);
    offset.resize(attributesCount);
    stride.resize(attributesCount);
    for (size_t i = 0; i < attributesCount; ++i) {
        combined[i] = primitive.attributes[i].pVertexBuffer.Get();
        stride[i] = primitive.attributes[i].byteStride;
    }
    context->IASetVertexBuffers(0, static_cast<UINT>(attributesCount), combined.data(), stride.data(), offset.data());

    context->IASetIndexBuffer(primitive.pIndexBuffer.Get(), primitive.indexFormat, 0);
    context->IASetPrimitiveTopology(primitive.primitiveTopology);

    GeomPrimitiveMaterial& material = m_materials[primitive.material];
    if (material.blend) {
        context->OMSetBlendState(material.pBlendState.Get(), nullptr, 0xFFFFFFFF);
    }
    context->RSSetState(material.pRasterizerState.Get());

    context->IASetInputLayout(material.pInputLayout.Get());
    context->VSSetShader(material.pVertexShader.Get(), nullptr, 0);
    context->PSSetShader(material.pPixelShader.Get(), nullptr, 0);

    DirectX::XMStoreFloat4x4(&cameraData.model, DirectX::XMMatrixTranspose(m_worldMatricies[primitive.matrix]));

    context->UpdateSubresource(cameraConstantBuffer, 0, NULL, &cameraData, 0, 0);
    context->UpdateSubresource(lightConstantBuffer, 0, NULL, &lightData, 0, 0);
    context->UpdateSubresource(generalConstantBuffer, 0, NULL, &generalData, 0, 0);
    context->UpdateSubresource(m_geomMaterialMaterialConstantBuffer.Get(), 0, NULL, &material.materialBufferData, 0, 0);

    context->PSSetShaderResources(0, 1, &irradianceMapSRV);
    context->PSSetShaderResources(1, 1, &prefilteredColorMapSRV);
    context->PSSetShaderResources(2, 1, &preintegratedBRDFSRV);

    if (material.baseColorTexture >= 0) {
        context->PSSetShaderResources(3, 1, m_pShaderResourceViews[material.baseColorTexture].GetAddressOf());
    }
    if (material.metallicRoughnessTexture >= 0) {
        context->PSSetShaderResources(4, 1, m_pShaderResourceViews[material.metallicRoughnessTexture].GetAddressOf());
    }
    if (material.normalTexture >= 0) {
        context->PSSetShaderResources(5, 1, m_pShaderResourceViews[material.normalTexture].GetAddressOf());
    }

    context->DrawIndexed(primitive.indexCount, 0, 0);

    if (material.blend) {
        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    }
}
