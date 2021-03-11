#include "PBRInclude.cginc"

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 wo = toCamera(input.worldPos);

    float3 finalColor =
        albedo * 0.56f +
        Lo(0, input.worldPos, input.normal, wo) +
        Lo(1, input.worldPos, input.normal, wo);
        //Lo(2, input.worldPos, input.normal, wo);

    return float4(finalColor, 1.0f);
}
