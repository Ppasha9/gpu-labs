#include "PBRInclude.cginc"

float4 main(PixelShaderInput input) : SV_TARGET
{
    return normalDistribution(
        normalize(input.normal),
        toLight(0, input.worldPos),
        toCamera(input.worldPos)
    );
}
