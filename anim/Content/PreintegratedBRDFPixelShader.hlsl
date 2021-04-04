#include "PBRInclude.cginc"
#include "ImportanceSample.cginc"

TextureCube environmentMap;

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float k = sqr(roughness) / 2;

    return SchlickGGX(N, V, k) * SchlickGGX(N, L, k);
}

float2 IntegrateBRDF(float NdotV, float roughness)
{
    // Calculate V using NdotV, with N=(0,1,0)
    float3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.z = 0.0;
    V.y = NdotV;
    // Desired coefficients
    float A = 0.0;
    float B = 0.0;
    float3 N = float3(0.0, 1.0, 0.0);
    // Random H vector generation
    static const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(L.y, 0.0);
        float NdotH = max(H.y, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        if (NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    return float2(A, B);
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    return float4(IntegrateBRDF(input.color.x, input.color.y), 0, 1);
}
