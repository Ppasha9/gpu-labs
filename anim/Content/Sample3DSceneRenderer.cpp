#include "pch.h"

#include "Sample3DSceneRenderer.h"
#include "WICTextureLoader.h"

#include "..\Common\DirectXHelper.h"
#include "..\Common\StepTimer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "..\Common\stb_image.h"

using namespace anim;

using namespace DirectX;
using namespace Windows::Foundation;
using namespace Microsoft::WRL;

// Loads vertex and pixel shaders from files and instantiates the sphere geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(
    const std::shared_ptr<DX::DeviceResources>& deviceResources,
    const std::shared_ptr<Camera>& camera,
    const std::shared_ptr<input::Keyboard>& keyboard) :
    m_indexCount(0),
    m_deviceResources(deviceResources),
    m_camera(camera),
    m_keyboard(keyboard),
    m_shaderMode(PBRShaderMode::REGULAR)
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
    DX::Size outputSize = m_deviceResources->GetLogicalSize();
    float aspectRatio = outputSize.width / outputSize.height;

    m_camera->SetProjectionValues(70.0f, aspectRatio, 0.01f, 1000.0f);

    XMStoreFloat4x4(
        &m_constantBufferData.projection,
        XMMatrixTranspose(m_camera->GetProjectionMatrix())
    );

    // Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
    static const XMFLOAT3 eye = { 0.0f, 0.0f, 5.0f };
    static const XMFLOAT3 at = { 0.0f, 0.0f, 0.0f };

    m_camera->SetPosition(XMLoadFloat3(&eye));
    m_camera->SetLookAtPos(at);

    XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(m_camera->GetViewMatrix()));
}

void Sample3DSceneRenderer::CycleLight(int lightId)
{
    static int lightState[3] = { 0, 0, 0 };
    static const float strengths[] = { 0, 10, 100, 300, 1000 };
    static const int stateN = sizeof(strengths) / sizeof(float);
    auto nextState = [](int curState) -> int
    {
        return ++curState >= stateN ? 0 : curState;
    };

    lightState[lightId] = nextState(lightState[lightId]);
    m_strengths[lightId] = strengths[lightState[lightId]];
}

void Sample3DSceneRenderer::SetMaterial(MaterialConstantBuffer material)
{
    m_materialConstantBufferData = material;

    auto context = m_deviceResources->GetD3DDeviceContext();

    context->UpdateSubresource(m_materialConstantBuffer.Get(), 0, NULL,
        &m_materialConstantBufferData, 0, 0);
    context->PSSetConstantBuffers(1, 1, m_materialConstantBuffer.GetAddressOf());
}

void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
    if (m_keyboard->KeyWasReleased('1'))
        CycleLight(0);
    if (m_keyboard->KeyWasReleased('2'))
        CycleLight(1);
    //if (m_keyboard->KeyWasReleased('3'))
    //    CycleLight(2);

    if (m_keyboard->KeyWasReleased('3'))
        m_shaderMode = PBRShaderMode::REGULAR;
    if (m_keyboard->KeyWasReleased('4'))
        m_shaderMode = PBRShaderMode::NORMAL_DISTRIBUTION;
    if (m_keyboard->KeyWasReleased('5'))
        m_shaderMode = PBRShaderMode::GEOMETRY;
    if (m_keyboard->KeyWasReleased('6'))
        m_shaderMode = PBRShaderMode::FRESNEL;
    if (m_keyboard->KeyWasReleased('7'))
        m_isDrawIrradiance = !m_isDrawIrradiance;

    // Update the view matrix, cause it can be changed by input
    XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(m_camera->GetViewMatrix()));

    m_lightConstantBufferData.lightColor1 = XMFLOAT4(LIGHT_COLOR_1.x, LIGHT_COLOR_1.y, LIGHT_COLOR_1.z, m_strengths[0]);
    m_lightConstantBufferData.lightColor2 = XMFLOAT4(LIGHT_COLOR_2.x, LIGHT_COLOR_2.y, LIGHT_COLOR_2.z, m_strengths[1]);
    m_lightConstantBufferData.lightColor3 = XMFLOAT4(LIGHT_COLOR_3.x, LIGHT_COLOR_3.y, LIGHT_COLOR_3.z, m_strengths[2]);

    m_generalConstantBufferData.cameraPos = m_camera->GetPositionFloat3();
    m_generalConstantBufferData.time = (float)timer.GetTotalSeconds();
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto annotation = m_deviceResources->GetAnnotation();

    // Prepare the constant buffer to send it to the graphics device.
    context->UpdateSubresource(m_lightConstantBuffer.Get(), 0, NULL,
        &m_lightConstantBufferData, 0, 0);
    context->UpdateSubresource(m_generalConstantBuffer.Get(), 0, NULL,
        &m_generalConstantBufferData, 0, 0);

    // Each vertex is one instance of the VertexPositionColor struct.
    UINT stride = sizeof(VertexPositionColorNormal);
    UINT offset = 0;
    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());

    // Send the constant buffer to the graphics device.
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_lightConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(2, 1, m_generalConstantBuffer.GetAddressOf());

    annotation->BeginEvent(L"RenderSkySphere");
    // Set sky sphere texture and shaders
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_skySpherePixelShader.Get(), nullptr, 0);

    if (m_isDrawIrradiance)
        context->PSSetShaderResources(0, 1, m_irradianceMapSRV.GetAddressOf());
    else
        context->PSSetShaderResources(0, 1, m_skyMapSRV.GetAddressOf());
    context->PSSetSamplers(0, 1, m_deviceResources->GetSamplerState());

    // Set sky sphere geometry
    context->IASetVertexBuffers(0, 1, m_skySphereVertexBuffer.GetAddressOf(), &stride, &offset);
    // Set scale matrix
    XMStoreFloat4x4(
        &m_constantBufferData.model,
        XMMatrixMultiplyTranspose(
            XMMatrixScaling(
                999,
                999,
                999
            ),
            XMMatrixTranslationFromVector(m_camera->GetPositionVector())
        )
    );
    context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);

    // Render sky sphere
    context->DrawIndexed(
        (UINT)m_indexCount,
        0,
        0
    );
    annotation->EndEvent(); // RenderSkySphere

    annotation->BeginEvent(L"RenderSphereGrid");

    // Set sphere geometry
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    // Attach our vertex shader.
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    // Attach our pixel shader.
    switch (m_shaderMode)
    {
    case PBRShaderMode::REGULAR:
        context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
        break;
    case PBRShaderMode::NORMAL_DISTRIBUTION:
        context->PSSetShader(m_normDistrPixelShader.Get(), nullptr, 0);
        break;
    case PBRShaderMode::GEOMETRY:
        context->PSSetShader(m_geomPixelShader.Get(), nullptr, 0);
        break;
    case PBRShaderMode::FRESNEL:
        context->PSSetShader(m_fresnelPixelShader.Get(), nullptr, 0);
        break;
    }
    // Bind irradiance map
    context->PSSetShaderResources(0, 1, m_irradianceMapSRV.GetAddressOf());

    static const int sphereGridSize = 10;
    static const float gridWidth = 5;
    for (int i = 0; i < sphereGridSize; i++)
        for (int j = 0; j < sphereGridSize; j++)
        {
            XMStoreFloat4x4(
                &m_constantBufferData.model,
                XMMatrixTranspose(XMMatrixTranslation(
                    gridWidth * (i / (sphereGridSize - 1.0f) - 0.5f),
                    gridWidth * (j / (sphereGridSize - 1.0f) - 0.5f),
                    0
                ))
            );
            context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);

            SetMaterial(
                {
                    XMFLOAT3(1, 1, 1),
                    i / (sphereGridSize - 1.0f),
                    j / (sphereGridSize - 1.0f)
                }
            );

            context->DrawIndexed(
                (UINT)m_indexCount,
                0,
                0
            );
        }

    annotation->EndEvent();
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
    // Load shaders
    std::vector<byte> vsData;
    bool success = false;
    try
    {
#ifdef _DEBUG
        vsData = DX::ReadData("x64\\Debug\\SampleVertexShader.cso");
#else
        vsData = DX::ReadData("x64\\Release\\SampleVertexShader.cso");
#endif // DEBUG
        success = true;
    }
    catch (std::exception &)
    {
    }
    if (!success)
    {
        vsData = DX::ReadData("SampleVertexShader.cso");
    }

    auto device = m_deviceResources->GetD3DDevice();

    // After the vertex shader file is loaded, create the shader and input layout.
    DX::ThrowIfFailed(
        device->CreateVertexShader(
            vsData.data(),
            vsData.size(),
            nullptr,
            &m_vertexShader
        )
    );
    DX::SetName(m_vertexShader, "SampleVertexShader");

    static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    DX::ThrowIfFailed(
        device->CreateInputLayout(
            vertexDesc,
            ARRAYSIZE(vertexDesc),
            vsData.data(),
            vsData.size(),
            &m_inputLayout
        )
    );

    m_unlitVertexShader = m_deviceResources->createVertexShader("Unlit");

    // Create pixel shader
    m_skySpherePixelShader = m_deviceResources->createPixelShader("SkySphere");
    m_unlitPixelShader = m_deviceResources->createPixelShader("Unlit");
    m_pixelShader = m_deviceResources->createPixelShader("PBR");
    m_normDistrPixelShader = m_deviceResources->createPixelShader("NormalDistribution");
    m_geomPixelShader = m_deviceResources->createPixelShader("Geometry");
    m_fresnelPixelShader = m_deviceResources->createPixelShader("Fresnel");

    // Create MVP matrices constast buffer
    CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    DX::ThrowIfFailed(
        device->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &m_constantBuffer
        )
    );

    CD3D11_BUFFER_DESC lightConstantBufferDesc(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    DX::ThrowIfFailed(
        device->CreateBuffer(
            &lightConstantBufferDesc,
            nullptr,
            &m_lightConstantBuffer
        )
    );

    CD3D11_BUFFER_DESC materialConstantBufferDesc(sizeof(MaterialConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    DX::ThrowIfFailed(
        device->CreateBuffer(
            &materialConstantBufferDesc,
            nullptr,
            &m_materialConstantBuffer
        )
    );

    CD3D11_BUFFER_DESC generalConstantBufferDesc(sizeof(GeneralConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    DX::ThrowIfFailed(
        device->CreateBuffer(
            &generalConstantBufferDesc,
            nullptr,
            &m_generalConstantBuffer
        )
    );

    // Create sphere geometry
    static const int
        numLatitudeLines = 16,
        numLongitudeLines = 16;
    static const float radius = 0.5f;
    static const float latitudeSpacing = 1.0f / numLatitudeLines;
    static const float longitudeSpacing = 1.0f / numLongitudeLines;
    static const float PI = 3.141592653589793238463f;

    std::vector<VertexPositionColorNormal> vertices;
    for (int latitude = 0; latitude <= numLatitudeLines; latitude++)
    {
        for (int longitude = 0; longitude <= numLongitudeLines; longitude++)
        {
            VertexPositionColorNormal v;

            // Scale coordinates into the 0...1 texture coordinate range,
            // with north at the top (y = 1).
            v.color = XMFLOAT3(
                longitude * longitudeSpacing,
                1.0f - latitude * latitudeSpacing,
                0
            );

            // Convert to spherical coordinates:
            // theta is a longitude angle (around the equator) in radians.
            // phi is a latitude angle (north or south of the equator).
            float theta = v.color.x * 2.0f * PI;
            float phi = (v.color.y - 0.5f) * PI;

            float c = cos(phi);

            v.normal = XMFLOAT3(
                c * cos(theta) * radius,
                sin(phi) * radius,
                c * sin(theta) * radius
            );

            v.pos = XMFLOAT3(
                v.normal.x * radius,
                v.normal.y * radius,
                v.normal.z * radius
            );

            vertices.push_back(v);
        }
    }
    m_vertexBuffer = m_deviceResources->createVertexBuffer(vertices, "Sphere");

    // Create sky sphere vertex buffer
    for (auto &v : vertices)
    {
        v.pos.x = -v.pos.x;
        v.pos.y = -v.pos.y;
        v.pos.z = -v.pos.z;
    
        v.normal.x = -v.normal.x;
        v.normal.y = -v.normal.y;
        v.normal.z = -v.normal.z;
    }
    m_skySphereVertexBuffer =
        m_deviceResources->createVertexBuffer(vertices, "SkySphere");

    // Create sphere index buffer
    std::vector<unsigned short> indices;
    for (int latitude = 0; latitude < numLatitudeLines; latitude++)
    {
        for (int longitude = 0; longitude < numLongitudeLines; longitude++)
        {
            indices.push_back(latitude * (numLongitudeLines + 1) + longitude);
            indices.push_back((latitude + 1) * (numLongitudeLines + 1) + longitude);
            indices.push_back(latitude * (numLongitudeLines + 1) + (longitude + 1));

            indices.push_back(latitude * (numLongitudeLines + 1) + (longitude + 1));
            indices.push_back((latitude + 1) * (numLongitudeLines + 1) + longitude);
            indices.push_back((latitude + 1) * (numLongitudeLines + 1) + (longitude + 1));
        }
    }

    m_indexCount = indices.size();
    m_indexBuffer = m_deviceResources->createIndexBuffer(indices, "Sphere");

    // Load sky sphere texture
    struct STBImage
    {
        int w;
        int h;
        int comp;
        float *data = nullptr;

        HRESULT load(const std::string &filename)
        {
            data = stbi_loadf(filename.c_str(), &w, &h, &comp, STBI_rgb_alpha);
            if (data == nullptr)
                return 1;
            return 0;
        }

        ~STBImage()
        {
            stbi_image_free(data);
        }
    };

    auto loadTexture = [this](const std::string &filename)
    {
        STBImage image;
        DX::ThrowIfFailed(image.load(filename));

        CD3D11_TEXTURE2D_DESC desc(
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            image.w,
            image.h,
            1, // Only one texture.
            1, // Use a single mipmap level.
            D3D11_BIND_SHADER_RESOURCE
        );
        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = image.data;
        initData.SysMemPitch = 4 * image.w * sizeof(float);

        m_loadedSkyTextureSRV = m_deviceResources->createShaderResourceView(
            m_deviceResources->createTexture2D(
                desc,
                filename,
                &initData),
            filename);
    };

    success = false;
    try
    {
        loadTexture("skysphere.hdr");
        success = true;
    }
    catch (std::exception &)
    {
    }
    if (!success)
    {
        loadTexture("..\\..\\skysphere.hdr");
    }

    renderSkyMapTexture();
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
    m_vertexShader.Reset();
    m_inputLayout.Reset();
    m_pixelShader.Reset();
    m_normDistrPixelShader.Reset();
    m_geomPixelShader.Reset();
    m_fresnelPixelShader.Reset();
    m_constantBuffer.Reset();
    m_lightConstantBuffer.Reset();
    m_materialConstantBuffer.Reset();
    m_generalConstantBuffer.Reset();
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
}

void Sample3DSceneRenderer::renderSkyMapTexture()
{
    static const UINT FACE_SIZE = 512;

    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto annotation = m_deviceResources->GetAnnotation();

    annotation->BeginEvent(L"RenderSkyMap");

    // Load pixel shader
    ComPtr<ID3D11PixelShader> pixelShader = m_deviceResources->createPixelShader("Equidistant2CubeMap");

    // Create cubemap texture
    CD3D11_TEXTURE2D_DESC cubeMapDesc(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        FACE_SIZE,
        FACE_SIZE,
        6, // Six textures for faces.
        1, // Use a single mipmap level.
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT, 0, 1, 0,
        D3D11_RESOURCE_MISC_TEXTURECUBE
    );
    m_skyCubeMap = m_deviceResources->createTexture2D(cubeMapDesc, "SkyCubeMap");

    // Create face-sized render target
    DX::RenderTargetTexture rt =
        m_deviceResources->createRenderTargetTexture(
            { FACE_SIZE, FACE_SIZE }, "CubeMapRenderTexture"
        );
    D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.0f, 0.0f, FACE_SIZE, FACE_SIZE);

    // Create full-screen quad
    static const std::vector<VertexPositionColorNormal> vertices(
        {
            { { -0.5f,  0.5f, -0.5f }, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
            { {  0.5f,  0.5f, -0.5f }, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
            { {  0.5f, -0.5f, -0.5f }, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} },
            { { -0.5f, -0.5f, -0.5f }, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }
        }
    );
    static const std::vector<unsigned short> indices(
        {
            2, 0, 1,
            2, 3, 0
        }
    );
    ComPtr<ID3D11Buffer> vertexBuffer =
        m_deviceResources->createVertexBuffer(vertices, "CubeMapFullScreenQuad");
    ComPtr<ID3D11Buffer> indexBuffer =
        m_deviceResources->createIndexBuffer(indices, "CubeMapFullScreenQuad");

    // Quad rotation matrices
    static const XMMATRIX rotations[6] =
    {
        XMMatrixTranspose(XMMatrixRotationY(-XM_PI / 2)), // +x
        XMMatrixTranspose(XMMatrixRotationY( XM_PI / 2)), // -x
        XMMatrixTranspose(XMMatrixRotationX( XM_PI / 2)), // +y
        XMMatrixTranspose(XMMatrixRotationX(-XM_PI / 2)), // -y
        XMMatrixIdentity(),                               // +z (in DirectX left-handed system)
        XMMatrixTranspose(XMMatrixRotationY(XM_PI))       // -z (in DirectX left-handed system)
    };

    // Create camera
    Camera cubeMapCamera;
    cubeMapCamera.SetProjectionValues(90.0f, 1.0f, 0.01f, 1000.0f);
    XMStoreFloat4x4(
        &m_constantBufferData.projection,
        XMMatrixTranspose(cubeMapCamera.GetProjectionMatrix())
    );
    cubeMapCamera.SetPosition(0.0f, 0.0f, 0.0f);

    XMFLOAT3 lookAt[6] =
    {
        { 1.0f, 0.0f, 0.0f}, // +x
        {-1.0f, 0.0f, 0.0f}, // -x
        {0.0f,  1.0f, 0.0f}, // +y
        {0.0f, -1.0f, 0.0f}, // -y
        {0.0f, 0.0f, -1.0f}, // +z (in DirectX left-handed system)
        {0.0f, 0.0f,  1.0f}  // -z (in DirectX left-handed system)
    };

    const XMVECTORF32 clrs[6] = {
        DirectX::Colors::Red,
        DirectX::Colors::Purple,
        DirectX::Colors::Green,
        DirectX::Colors::Yellow,
        DirectX::Colors::Blue,
        DirectX::Colors::Cyan
    };

    // Set rendering parameters
    UINT stride = sizeof(VertexPositionColorNormal);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());

    context->OMSetRenderTargets(1, rt.renderTargetView.GetAddressOf(), nullptr);
    context->RSSetViewports(1, &viewport);

    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->PSSetSamplers(0, 1, m_deviceResources->GetSamplerState());
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    context->PSSetShaderResources(0, 1, m_loadedSkyTextureSRV.GetAddressOf());

    // Render each face
    auto renderFace = [&](int i, ComPtr<ID3D11Texture2D> targetCubeMapTexture)
    {
        // Set camera look at
        cubeMapCamera.SetLookAtPos(lookAt[i]);
        XMStoreFloat4x4(
            &m_constantBufferData.view,
            XMMatrixTranspose(cubeMapCamera.GetViewMatrix())
        );

        // Set full-screen quad rotation
        XMStoreFloat4x4(&m_constantBufferData.model, rotations[i]);

        // Send constant buffer data to GPU
        context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL,
            &m_constantBufferData, 0, 0);

        // Render full-screen quad
        context->ClearRenderTargetView(rt.renderTargetView.Get(), clrs[i]);
        context->DrawIndexed((UINT)indices.size(), 0, 0);

        // Copy render target contents to cube map
        context->CopySubresourceRegion(
            targetCubeMapTexture.Get(), i,
            0, 0, 0,
            rt.texture.Get(), 0,
            nullptr
        );
    };

    for (int i = 0; i < 6; i++)
        renderFace(i, m_skyCubeMap);

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC desc =  CD3D11_SHADER_RESOURCE_VIEW_DESC(
        m_skyCubeMap.Get(),
        D3D11_SRV_DIMENSION_TEXTURECUBE,
        cubeMapDesc.Format,
        0,
        1
    );

    m_skyMapSRV = m_deviceResources->createShaderResourceView(
        m_skyCubeMap,
        "SkyMap",
        &desc
    );

    annotation->EndEvent(); // RenderSkyMap

    annotation->BeginEvent(L"RenderIrradianceMap");
    static const UINT IRR_FACE_SIZE = 32;

    // Load irradiance pixel shader
    pixelShader = m_deviceResources->createPixelShader("IrradianceMap");

    // Create irradiance cubemap texture
    CD3D11_TEXTURE2D_DESC irrCubeMapDesc(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        IRR_FACE_SIZE,
        IRR_FACE_SIZE,
        6, // Six textures for faces.
        1, // Use a single mipmap level.
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT, 0, 1, 0,
        D3D11_RESOURCE_MISC_TEXTURECUBE
    );
    m_irradianceCubeMap = m_deviceResources->createTexture2D(irrCubeMapDesc, "IrradianceCubeMap");

    // Create face-sized render target
    rt =  m_deviceResources->createRenderTargetTexture(
        { IRR_FACE_SIZE, IRR_FACE_SIZE }, "IrradianceMapRenderTexture"
    );
    viewport = CD3D11_VIEWPORT(0.0f, 0.0f, IRR_FACE_SIZE, IRR_FACE_SIZE);

    // Set rendering parameters
    context->OMSetRenderTargets(1, rt.renderTargetView.GetAddressOf(), nullptr);
    context->RSSetViewports(1, &viewport);
    context->PSSetShader(pixelShader.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, m_skyMapSRV.GetAddressOf());

    // Render each face
    for (int i = 0; i < 6; i++)
        renderFace(i, m_irradianceCubeMap);

    // Create shader resource view
    m_irradianceMapSRV = m_deviceResources->createShaderResourceView(
        m_irradianceCubeMap,
        "IrradianceMap",
        &desc
    );

    annotation->EndEvent(); // RenderIrradianceMap

    // Restore main camera matrices
    XMStoreFloat4x4(
        &m_constantBufferData.view,
        XMMatrixTranspose(m_camera->GetViewMatrix())
    );
    XMStoreFloat4x4(
        &m_constantBufferData.projection,
        XMMatrixTranspose(m_camera->GetProjectionMatrix())
    );
}
