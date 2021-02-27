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
    float4 textureColor = shaderTexture.Sample(samplerState, input.texcoord);
    float l = max(max(textureColor.r, textureColor.g), textureColor.b);
    return log(l) + 1;
}