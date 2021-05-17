#pragma once

HRESULT CompileShaderFromFile (const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, D3D_SHADER_MACRO* pDefines);
