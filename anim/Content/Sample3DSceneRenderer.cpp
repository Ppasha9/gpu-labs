#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"
#include "..\Common\StepTimer.h"

using namespace anim;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(
    const std::shared_ptr<DX::DeviceResources>& deviceResources,
    const std::shared_ptr<Camera>& camera,
    const std::shared_ptr<input::Keyboard>& keyboard) :
    m_degreesPerSecond(45),
    m_indexCount(0),
    m_tracking(false),
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

    m_camera->SetProjectionValues(70.0f, aspectRatio, 0.01, 100.0f);

    XMStoreFloat4x4(
        &m_constantBufferData.projection,
        XMMatrixTranspose(m_camera->GetProjectionMatrix())
    );

    // Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
    static const XMFLOAT3 eye = { 0.0f, 0.7f, 1.5f };
    static const XMFLOAT3 at = { 0.0f, -0.1f, 0.0f };

    m_camera->SetPosition(XMLoadFloat3(&eye));
    m_camera->SetLookAtPos(at);

    XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(m_camera->GetViewMatrix()));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
    if (m_keyboard->KeyIsPressed('T')) {
        m_strengths[0] = 1.0f;
    } else if (m_keyboard->KeyIsPressed('Y')) {
        m_strengths[0] = 10.0f;
    } else if (m_keyboard->KeyIsPressed('U')) {
        m_strengths[0] = 100.0f;
    }

    if (m_keyboard->KeyIsPressed('G')) {
        m_strengths[1] = 1.0f;
    } else if (m_keyboard->KeyIsPressed('H')) {
        m_strengths[1] = 10.0f;
    } else if (m_keyboard->KeyIsPressed('J')) {
        m_strengths[1] = 100.0f;
    }

    if (m_keyboard->KeyIsPressed('B')) {
        m_strengths[2] = 1.0f;
    } else if (m_keyboard->KeyIsPressed('N')) {
        m_strengths[2] = 10.0f;
    } else if (m_keyboard->KeyIsPressed('M')) {
        m_strengths[2] = 100.0f;
    }

    // Convert degrees to radians, then convert seconds to rotation angle
    float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
    double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
    float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

    Rotate(radians);

    // Update the view matrix, cause it can be changed by input
    XMStoreFloat4x4(&m_constantBufferData.view, XMMatrixTranspose(m_camera->GetViewMatrix()));

    m_lightConstantBufferData.lightColor1 = XMFLOAT4(LIGHT_COLOR_1.x, LIGHT_COLOR_1.y, LIGHT_COLOR_1.z, m_strengths[0]);
    m_lightConstantBufferData.lightColor2 = XMFLOAT4(LIGHT_COLOR_2.x, LIGHT_COLOR_2.y, LIGHT_COLOR_2.z, m_strengths[1]);
    m_lightConstantBufferData.lightColor3 = XMFLOAT4(LIGHT_COLOR_3.x, LIGHT_COLOR_3.y, LIGHT_COLOR_3.z, m_strengths[2]);
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
    // Prepare to pass the updated model matrix to the shader
    // XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
    XMStoreFloat4x4(&m_constantBufferData.model, XMMatrixIdentity());
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    auto annotation = m_deviceResources->GetAnnotation();

    annotation->BeginEvent(L"SetCubeGeometry");

    // Prepare the constant buffer to send it to the graphics device.
    context->UpdateSubresource(
        m_constantBuffer.Get(),
        0,
        NULL,
        &m_constantBufferData,
        0,
        0
    );

    context->UpdateSubresource(
        m_lightConstantBuffer.Get(),
        0,
        NULL,
        &m_lightConstantBufferData,
        0,
        0
    );

    // Each vertex is one instance of the VertexPositionColor struct.
    UINT stride = sizeof(VertexPositionColorNormal);
    UINT offset = 0;
    context->IASetVertexBuffers(
        0,
        1,
        m_vertexBuffer.GetAddressOf(),
        &stride,
        &offset
    );

    context->IASetIndexBuffer(
        m_indexBuffer.Get(),
        DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
        0
    );

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->IASetInputLayout(m_inputLayout.Get());
    annotation->EndEvent();

    annotation->BeginEvent(L"AttachShaders");
    // Attach our vertex shader.
    context->VSSetShader(
        m_vertexShader.Get(),
        nullptr,
        0
    );

    // Send the constant buffer to the graphics device.
    context->VSSetConstantBuffers(
        0,
        1,
        m_constantBuffer.GetAddressOf()
    );

    // Attach our pixel shader.
    context->PSSetShader(
        m_pixelShader.Get(),
        nullptr,
        0
    );

    context->PSSetConstantBuffers(
        0,
        1,
        m_lightConstantBuffer.GetAddressOf()
    );

    annotation->EndEvent();

    annotation->BeginEvent(L"RenderCube");
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
    std::vector<byte> vsData, psData;

    bool success = false;
    try
    {
#ifdef _DEBUG
        vsData = DX::ReadData("x64\\Debug\\SampleVertexShader.cso");
        psData = DX::ReadData("x64\\Debug\\SamplePixelShader.cso");
#else
        vsData = DX::ReadData("x64\\Release\\SampleVertexShader.cso");
        psData = DX::ReadData("x64\\Release\\SamplePixelShader.cso");
#endif // DEBUG
        success = true;
    }
    catch (std::exception&)
    {
    }
    if (!success)
    {
        vsData = DX::ReadData("SampleVertexShader.cso");
        psData = DX::ReadData("SamplePixelShader.cso");
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

    // After the pixel shader file is loaded, create the shader and constant buffer.
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreatePixelShader(
            psData.data(),
            psData.size(),
            nullptr,
            &m_pixelShader
        )
    );

    shaderName = "SamplePixelShader";
    m_pixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)shaderName.size(),
        shaderName.c_str());

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

    // Load mesh vertices. Each vertex has a position, color and a normal.
    static const VertexPositionColorNormal cubeVertices[] =
    {
        {XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, -1.0f, -1.0f)},
        {XMFLOAT3(-0.5f, -0.5f,  0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(-1.0f, -1.0f, 1.0f)},
        {XMFLOAT3(-0.5f,  0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(-1.0f, 1.0f, -1.0f)},
        {XMFLOAT3(-0.5f,  0.5f,  0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 1.0f, 1.0f)},
        {XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, -1.0f, -1.0f)},
        {XMFLOAT3(0.5f, -0.5f,  0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, -1.0f, 1.0f)},
        {XMFLOAT3(0.5f,  0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, -1.0f)},
        {XMFLOAT3(0.5f,  0.5f,  0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
    };

    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    vertexBufferData.pSysMem = cubeVertices;
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &m_vertexBuffer
        )
    );

    std::string modelName = "Cube";
    m_vertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)modelName.size(),
        modelName.c_str());

    // Load mesh indices. Each trio of indices represents
    // a triangle to be rendered on the screen.
    // For example: 0,2,1 means that the vertices with indexes
    // 0, 2 and 1 from the vertex buffer compose the 
    // first triangle of this mesh.
    static const unsigned short cubeIndices[] =
    {
        0,2,1, // -x
        1,2,3,

        4,5,6, // +x
        5,7,6,

        0,1,5, // -y
        0,5,4,

        2,6,7, // +y
        2,7,3,

        0,4,6, // -z
        0,6,2,

        1,3,7, // +z
        1,7,5,
    };

    m_indexCount = ARRAYSIZE(cubeIndices);

    D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
    indexBufferData.pSysMem = cubeIndices;
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;
    CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
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
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
}