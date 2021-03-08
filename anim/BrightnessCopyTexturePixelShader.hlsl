Texture2D shaderTexture;
SamplerState samplerState;

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float4 p = shaderTexture.Sample(samplerState, input.texcoord);
    return log(0.2126 * p.r + 0.7151 * p.g + 0.0722 * p.b + 1);
}