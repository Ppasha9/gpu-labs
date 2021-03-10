#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"
#include "..\Common\StepTimer.h"

using namespace anim;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the sphere geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(
    const std::shared_ptr<DX::DeviceResources>& deviceResources,
    const std::shared_ptr<Camera>& camera,
    const std::shared_ptr<input::Keyboard>& keyboard) :
    m_indexCount(0),
    m_deviceResources(deviceResources),
    m_camera(camera),
    m_keyboard(keyboard)
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
    DX::Size outputSize = m_deviceResources->GetLogicalSize();
    float aspectRatio = outputSize.width / outputSize.height;
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    m_camera->SetProjectionValues(70.0f, aspectRatio, 0.01f, 100.0f);

    XMStoreFloat4x4(
        &m_constantBufferData.projection,
        XMMatrixTranspose(m_camera->GetProjectionMatrix())
    );

    // Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
    static const XMFLOAT3 eye = { 0.0f, 0.7f, 1.5f };
    static const XMFLOAT3 at = { 0.0f, 0.0f, 0.0f };

    m_camera->SetPosition(XMLoadFloat3(&eye));
    m_camera->SetLookAtPos(at);

    XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(m_camera->GetViewMatrix()));
}

void Sample3DSceneRenderer::CycleLight(int lightId)
{
    static int lightState[3] = { 0, 0, 0 };
    static const float strengths[] = { 1, 10, 100 };
    static const int stateN = sizeof(strengths) / sizeof(float);
    auto nextState = [](int curState) -> int
    {
        return ++curState >= stateN ? 0 : curState;
    };

    lightState[lightId] = nextState(lightState[lightId]);
    m_strengths[lightId] = strengths[lightState[lightId]];
}

// Called once per frame, rotates the sphere and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
    if (m_keyboard->KeyWasReleased('1'))
        CycleLight(0);
    if (m_keyboard->KeyWasReleased('2'))
        CycleLight(1);
    if (m_keyboard->KeyWasReleased('3'))
        CycleLight(2);

    // Update the view matrix, cause it can be changed by input
    XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(m_camera->GetViewMatrix()));

    XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());

    m_lightConstantBufferData.lightColor1 = XMFLOAT4(LIGHT_COLOR_1.x, LIGHT_COLOR_1.y, LIGHT_COLOR_1.z, m_strengths[0]);
    m_lightConstantBufferData.lightColor2 = XMFLOAT4(LIGHT_COLOR_2.x, LIGHT_COLOR_2.y, LIGHT_COLOR_2.z, m_strengths[1]);
    m_lightConstantBufferData.lightColor3 = XMFLOAT4(LIGHT_COLOR_3.x, LIGHT_COLOR_3.y, LIGHT_COLOR_3.z, m_strengths[2]);

    m_generalConstantBufferData.cameraPos = m_camera->GetPositionFloat3();
    m_generalConstantBufferData.time = (float)timer.GetTotalSeconds();

    m_materialConstantBufferData.albedo = XMFLOAT3(1, 0, 0);
    m_materialConstantBufferData.roughness = 0.5f * (1 + sin(timer.GetTotalSeconds()));
    m_materialConstantBufferData.metalness = 0.5f * (1 + cos(timer.GetTotalSeconds() / 2));
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    auto annotation = m_deviceResources->GetAnnotation();

    annotation->BeginEvent(L"SetSphereGeometry");

    // Prepare the constant buffer to send it to the graphics device.
    context->UpdateSubresource(m_constantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);
    context->UpdateSubresource(m_lightConstantBuffer.Get(), 0, NULL,
        &m_lightConstantBufferData, 0, 0);
    context->UpdateSubresource(m_materialConstantBuffer.Get(), 0, NULL,
        &m_materialConstantBufferData, 0, 0);
    context->UpdateSubresource(m_generalConstantBuffer.Get(), 0, NULL,
        &m_generalConstantBufferData, 0, 0);

    // Each vertex is one instance of the VertexPositionColor struct.
    UINT stride = sizeof(VertexPositionColorNormal);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->IASetInputLayout(m_inputLayout.Get());
    annotation->EndEvent();

    annotation->BeginEvent(L"AttachShaders");
    // Attach our vertex shader.
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

    // Send the constant buffer to the graphics device.
    context->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    // Attach our pixel shader.
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    context->PSSetConstantBuffers(0, 1, m_lightConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(1, 1, m_materialConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(2, 1, m_generalConstantBuffer.GetAddressOf());

    annotation->EndEvent();

    annotation->BeginEvent(L"RenderSphere");
    // Draw the objects.
    context->DrawIndexed(
        (UINT)m_indexCount,
        0,
        0
    );
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
    catch (std::exception&)
    {
    }
    if (!success)
    {
        vsData = DX::ReadData("SampleVertexShader.cso");
    }

    // After the vertex shader file is loaded, create the shader and input layout.
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateVertexShader(
            vsData.data(),
            vsData.size(),
            nullptr,
            &m_vertexShader
        )
    );

    std::string shaderName = "SampleVertexShader";
    m_vertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)shaderName.size(),
        shaderName.c_str());

    static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateInputLayout(
            vertexDesc,
            ARRAYSIZE(vertexDesc),
            vsData.data(),
            vsData.size(),
            &m_inputLayout
        )
    );

    // Create pixel shader
    m_pixelShader = m_deviceResources->createPixelShader("PBR");

    // Create MVP matrices constast buffer
    CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &constantBufferDesc,
            nullptr,
            &m_constantBuffer
        )
    );

    CD3D11_BUFFER_DESC lightConstantBufferDesc(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &lightConstantBufferDesc,
            nullptr,
            &m_lightConstantBuffer
        )
    );

    CD3D11_BUFFER_DESC materialConstantBufferDesc(sizeof(MaterialConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &materialConstantBufferDesc,
            nullptr,
            &m_materialConstantBuffer
        )
    );

    CD3D11_BUFFER_DESC generalConstantBufferDesc(sizeof(GeneralConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &generalConstantBufferDesc,
            nullptr,
            &m_generalConstantBuffer
        )
    );

    static const int
        numLatitudeLines = 128,
        numLongitudeLines = 128;
    static const float radius = 0.5f;
    static const float latitudeSpacing = 1.0f / numLatitudeLines;
    static const float longitudeSpacing = 1.0f / numLongitudeLines;
    static const float PI = 3.141592653589793238463f;

    static std::vector<VertexPositionColorNormal> vertices;

    for(int latitude = 0; latitude <= numLatitudeLines; latitude++)
    {
        for(int longitude = 0; longitude <= numLongitudeLines; longitude++)
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

    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    vertexBufferData.pSysMem = vertices.data();
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC vertexBufferDesc((UINT)vertices.size() *
        sizeof(VertexPositionColorNormal), D3D11_BIND_VERTEX_BUFFER);
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &m_vertexBuffer
        )
    );

    std::string modelName = "Sphere";
    m_vertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)modelName.size(),
        modelName.c_str());

    static std::vector<unsigned short> indices;
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

    D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
    indexBufferData.pSysMem = indices.data();
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC indexBufferDesc((UINT)indices.size() * sizeof(short),
        D3D11_BIND_INDEX_BUFFER);
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &indexBufferDesc,
            &indexBufferData,
            &m_indexBuffer
        )
    );
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
    m_vertexShader.Reset();
    m_inputLayout.Reset();
    m_pixelShader.Reset();
    m_constantBuffer.Reset();
    m_lightConstantBuffer.Reset();
    m_materialConstantBuffer.Reset();
    m_generalConstantBuffer.Reset();
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
}