TextureCube skyMap;
SamplerState samplerState;

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR0;
    float3 normal : NORMAL;
    float3 worldPos : WORLD_POSITION;
};

cbuffer GeneralConstantBuffer : register(b2)
{
    float3 cameraPos;
    float time;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 coord = input.worldPos - cameraPos;
    return skyMap.Sample(samplerState, coord);
}
