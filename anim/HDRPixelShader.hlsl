Texture2D shaderTexture;
SamplerState samplerState;

cbuffer AverageBrightnessConstantBuffer : register(b0)
{
    float averageBrightness;
    float3 dummy;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float4 textureColor = shaderTexture.Sample(samplerState, input.texcoord);
    return textureColor;
    //return (0.30 * textureColor.r + 0.59 * textureColor.g + 0.11 * textureColor.b) * averageBrightness;
}