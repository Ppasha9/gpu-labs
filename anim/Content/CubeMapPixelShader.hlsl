Texture2D shaderTexture;
SamplerState samplerState;

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR0;
    float3 normal : NORMAL;
    float3 worldPos : WORLD_POSITION;
};

static const float PI = 3.14159265359f;

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 n = normalize(input.worldPos);

    float2 texcoord;
    texcoord.x = 1.0f - atan2(-n.z, n.x) / (2 * PI);
    texcoord.y = 0.5f - asin(n.y) / PI;

    return shaderTexture.Sample(samplerState, texcoord);
}
