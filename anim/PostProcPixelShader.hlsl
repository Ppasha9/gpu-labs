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

    return textureColor;
    // return float4(input.texcoord.x, input.texcoord.y, 0, 1);
    // return float4(1,0,0,1); //the red color
}