#pragma once

namespace DX
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::runtime_error(_com_error(hr).ErrorMessage());
        }
    }

    inline std::vector<byte> ReadData(const std::string& filename)
    {
        std::ifstream inFile(filename.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

        if (inFile.is_open())
        {
            std::vector<byte> returnBuffer;
            returnBuffer.resize((size_t)inFile.tellg());
            inFile.seekg(0, std::ios::beg);
            inFile.read(reinterpret_cast<char*>(returnBuffer.data()), returnBuffer.size());
            inFile.close();
            return returnBuffer;
        }
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        return {};
    }

    inline void SetName(Microsoft::WRL::ComPtr<ID3D11DeviceChild> resource,
        const std::string &name)
    {
        resource->SetPrivateData(WKPDID_D3DDebugObjectName,
            (UINT)name.size(), name.c_str());
    }

#if defined(_DEBUG)
    // Check for SDK Layer support.
    inline bool SdkLayersAvailable()
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
            0,
            D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
            nullptr,                    // Any feature level will do.
            0,
            D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
            nullptr,                    // No need to keep the D3D device reference.
            nullptr,                    // No need to know the feature level.
            nullptr                     // No need to keep the D3D device context reference.
            );

        return SUCCEEDED(hr);
    }
#endif
}
