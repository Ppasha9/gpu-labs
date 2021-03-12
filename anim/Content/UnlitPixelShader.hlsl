#include "UnlitInclude.cginc"

Texture2D shaderTexture;
SamplerState samplerState;

float4 main(PixelShaderInput input) : SV_TARGET
{
    return shaderTexture.Sample(samplerState, input.texcoord) * 13;
}
