TextureCube skyMap;
SamplerState samplerState;

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR0;
    float3 normal : NORMAL;
    float3 worldPos : WORLD_POSITION;
};

static const float PI = 3.14159265359f;
#define N1 600
#define N2 150

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 irradiance = float3(0.0, 0.0, 0.0);

    float3 normal = normalize(input.worldPos);
    normal.z = -normal.z;

    float3 dir = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 tangent = normalize(cross(dir, normal));
    float3 bitangent = cross(normal, tangent);
    for (int i = 0; i < N1; i++)
    {
        for (int j = 0; j < N2; j++)
        {
            float phi = i * (2 * PI / N1);
            float theta = j * (PI / 2 / N2);
            // Spherical to cartesian coordinates (in tangent space)
            float3 tangentSample = float3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );
            // Tangent to world space
            float3 sampleVec = tangentSample.x * tangent +
                tangentSample.y * bitangent +
                tangentSample.z * normal;

            irradiance += skyMap.Sample(samplerState, sampleVec).rgb * cos(theta) * sin(theta);
        }
    }

    irradiance = PI * irradiance / (N1 * N2);

    return float4(irradiance, 1);
}
