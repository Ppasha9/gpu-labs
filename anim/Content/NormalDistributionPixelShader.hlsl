#include "PBRInclude.cginc"

float4 main(PixelShaderInput input) : SV_TARGET
{
    return normalDistribution(
        input.normal,
        toLight(0, input.worldPos),
        toCamera(input.worldPos)
    );
}
