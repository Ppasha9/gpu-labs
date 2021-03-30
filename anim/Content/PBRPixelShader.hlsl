#include "PBRInclude.cginc"

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 wo = toCamera(input.worldPos);
    float3 n = normalize(input.normal);

    float3 finalColor =
        ambient(n, wo) +
        Lo(0, input.worldPos, n, wo) +
        Lo(1, input.worldPos, n, wo);
        //Lo(2, input.worldPos, input.normal, wo);

    return float4(finalColor, 1.0f);
}
