TextureCube irradianceMap : register(t0);
TextureCube prefilteredColorMap : register(t1);
Texture2D preintegratedBRDF : register(t2);

SamplerState samplerState;

static const float PI = 3.14159265359f;

static const float3 lightPos[3] =
{
    float3(0.0f, 0.0f, 3.0f),
    float3(2.0f, 1.0f, 1.0f),
    float3(0, 1.0f, 0.3f)
};

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR0;
    float3 normal : NORMAL;
    float3 worldPos : WORLD_POSITION;
};

// A constant buffer that stores the three light colors.
cbuffer LightConstantBuffer : register(b0)
{
    float4 lightColor[3];
};

cbuffer GeneralConstantBuffer : register(b2)
{
    float3 cameraPos;
    float time;
};

cbuffer MaterialConstantBuffer : register(b1)
{
    float3 albedo;
    float roughness;
    float metalness;
    float3 dummy;
};

float sqr(float x)
{
    return x * x;
}

float myDot(float3 a, float3 b)
{
    return max(dot(a, b), 0);
}

float3 h(float3 wi, float3 wo)
{
    return normalize(wi + wo);
}

float normalDistributionH(float3 n, float3 h)
{
    float roughSqr = sqr(max(roughness, 0.01f));
    return roughSqr / (PI * sqr(sqr(myDot(n, h)) * (roughSqr - 1) + 1));
}

float normalDistribution(float3 n, float3 wi, float3 wo)
{
    return normalDistributionH(n, h(wi, wo));
}

float SchlickGGX(float3 n, float3 v, float k)
{
    float nv = myDot(n, v);
    return nv / (nv * (1 - k) + k);
}

float geometry(float3 n, float3 wi, float3 wo)
{
    float k = sqr(roughness + 1) / 8;
    return SchlickGGX(n, wi, k) * SchlickGGX(n, wo, k);
}

float3 fresnel(float3 n, float3 wi, float3 wo)
{
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
    return (F0 + (1 - F0) * pow(1 - myDot(h(wi, wo), wo), 5)) * sign(myDot(wi, n));
}

float3 f0()
{
    return lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
}

float3 fresnelEnvironment(float3 n, float3 wo)
{
    float3 F0 = f0();
    return (F0 + (max(1 - roughness, F0) - F0) * pow(1 - myDot(n, wo), 5));
}

float3 ambient(float3 n, float3 wo)
{
    // specular
    float3 r = 2 * myDot(n, wo) * n - wo;
    static const float MAX_REFLECTION_LOD = 4.0;
    float3 prefilteredColor = prefilteredColorMap.SampleLevel(samplerState, r, roughness * MAX_REFLECTION_LOD).rgb;

    float2 envBRDF = preintegratedBRDF.Sample(samplerState, float2(dot(n, wo), roughness)).xy;
    float3 specular = prefilteredColor * (f0() * envBRDF.x + envBRDF.y);

    // diffused
    float3 irradiance = irradianceMap.Sample(samplerState, n).rgb;
    float3 diffuse = irradiance * albedo;

    float3 F = fresnelEnvironment(n, wo);
    float3 kS = F; // reflected
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS; // diffused
    kD *= 1.0f - metalness;

    return kD * diffuse + specular;
}

float3 cookTorranceBRDF(float3 n, float3 wi, float3 wo)
{
    float D = normalDistribution(n, wi, wo);
    float G = geometry(n, wi, wo);
    float3 F = fresnel(n, wi, wo);

    return (1 - F) * albedo / PI * (1 - metalness) +
        D * F * G / (0.001f + 4 * (myDot(wi, n) * myDot(wo, n)));
}

float3 Li(int lightIdx, float3 p)
{
    float attenuation = 1 / (1 + 0.30f * sqr(distance(p, lightPos[lightIdx])));

    return lightColor[lightIdx].rgb * lightColor[lightIdx].a * attenuation;
}

float3 toLight(int lightIdx, float3 p)
{
    return normalize(lightPos[lightIdx] - p);
}

float3 Lo(int lightIdx, float3 p, float3 n, float3 wo)
{
    float3 wi = toLight(lightIdx, p);
    return cookTorranceBRDF(n, wi, wo) * Li(lightIdx, p) * myDot(wi, n);
}

float3 toCamera(float3 worldPos)
{
    return normalize(cameraPos - worldPos);
}
