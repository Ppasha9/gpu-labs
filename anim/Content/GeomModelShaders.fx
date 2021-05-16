TextureCube irradianceMap : register(t0);
TextureCube prefilteredColorMap : register(t1);
Texture2D preintegratedBRDF : register(t2);

Texture2D<float4> diffuseTexture : register(t3);
Texture2D<float4> metallicRoughnessTexture : register(t4);
Texture2D<float4> normalTexture : register(t5);

SamplerState samplerState;
SamplerState ModelSampler : register(s2);

static const float PI = 3.14159265359f;

static const float3 lightPos[3] =
{
    float3(0.0f, 0.0f, 3.0f),
    float3(2.0f, 1.0f, 1.0f),
    float3(0, 1.0f, 0.3f)
};

struct VertexShaderInput
{
    float3 normal : NORMAL;
    float3 pos : POSITION;
    float2 tex : TEXCOORD_0;
};

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD_0;
    float3 normal : NORMAL;
    float3 worldPos : WORLD_POSITION;
};

cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
};

cbuffer LightConstantBuffer : register(b1)
{
    float4 lightColor[3];
};

cbuffer GeneralConstantBuffer : register(b2)
{
    float3 cameraPos;
    float time;
};

cbuffer MaterialConstantBuffer : register(b3)
{
    float4 albedo;
    float roughness;
    float metalness;
    float2 dummy;
};


float4 getAlbedo(float2 uv)
{
    float4 res = albedo;
#ifdef HAS_BASE_COLOR_TEXTURE
    res *= diffuseTexture.Sample(ModelSampler, uv);
#endif
#ifdef HAS_OCCLUSION_TEXTURE
    res.xyz *= metallicRoughnessTexture.Sample(ModelSampler, uv).r;
#endif
    return res;
}

float2 getMetalnessRoughness(float2 uv)
{
    float2 material = float2(metalness, roughness);
#ifdef HAS_METALLIC_ROUGHNESS_TEXTURE
    material *= metallicRoughnessTexture.Sample(ModelSampler, uv).bg;
#endif
    return material.xy;
}


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

float normalDistributionH(float3 n, float3 h, float roughness)
{
    float roughSqr = sqr(max(roughness, 0.01f));
    return roughSqr / (PI * sqr(sqr(myDot(n, h)) * (roughSqr - 1) + 1));
}

float normalDistribution(float3 n, float3 wi, float3 wo, float roughness)
{
    return normalDistributionH(n, h(wi, wo), roughness);
}

float SchlickGGX(float3 n, float3 v, float k)
{
    float nv = myDot(n, v);
    return nv / (nv * (1 - k) + k);
}

float geometry(float3 n, float3 wi, float3 wo, float roughness)
{
    float k = sqr(roughness + 1) / 8;
    return SchlickGGX(n, wi, k) * SchlickGGX(n, wo, k);
}

float3 fresnel(float3 n, float3 wi, float3 wo, float3 albedo, float metalness)
{
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
    return (F0 + (1 - F0) * pow(1 - myDot(h(wi, wo), wo), 5)) * sign(myDot(wi, n));
}

float3 f0(float3 albedo, float metalness)
{
    return lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);
}

float3 fresnelEnvironment(float3 n, float3 wo, float3 albedo, float roughness, float metalness)
{
    float3 F0 = f0(albedo, metalness);
    return (F0 + (max(1 - roughness, F0) - F0) * pow(1 - myDot(n, wo), 5));
}

float3 ambient(float3 n, float3 wo, float3 albedo, float roughness, float metalness)
{
    // specular
    float3 r = 2 * myDot(n, wo) * n - wo;
    static const float MAX_REFLECTION_LOD = 4.0;
    float3 prefilteredColor = prefilteredColorMap.SampleLevel(samplerState, r, roughness * MAX_REFLECTION_LOD).rgb;

    float2 envBRDF = preintegratedBRDF.Sample(samplerState, float2(dot(n, wo), roughness)).xy;
    float3 specular = prefilteredColor * (f0(albedo, metalness) * envBRDF.x + envBRDF.y);

    // diffused
    float3 irradiance = irradianceMap.Sample(samplerState, n).rgb;
    float3 diffuse = irradiance * albedo;

    float3 F = fresnelEnvironment(n, wo, albedo, roughness, metalness);
    float3 kS = F; // reflected
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS; // diffused
    kD *= 1.0f - metalness;

    return kD * diffuse + specular;
}

float3 cookTorranceBRDF(float3 n, float3 wi, float3 wo, float3 albedo, float roughness, float metalness)
{
    float D = normalDistribution(n, wi, wo, roughness);
    float G = geometry(n, wi, wo, roughness);
    float3 F = fresnel(n, wi, wo, albedo, metalness);

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

float3 Lo(int lightIdx, float3 p, float3 n, float3 wo, float3 albedo, float roughness, float metalness)
{
    float3 wi = toLight(lightIdx, p);
    return cookTorranceBRDF(n, wi, wo, albedo, roughness, metalness) * Li(lightIdx, p) * myDot(wi, n);
}

float3 toCamera(float3 worldPos)
{
    return normalize(cameraPos - worldPos);
}


PixelShaderInput vs_main(VertexShaderInput input)
{
    PixelShaderInput output;
    float4 pos = float4(input.pos, 1.0f);

    pos = mul(pos, model);
    pos = mul(pos, view);
    pos = mul(pos, projection);
    output.pos = pos;

    output.worldPos = mul(float4(input.pos, 1.0f), model);
    output.normal = normalize(mul(input.normal, (float3x3)model));

    output.tex = input.tex;

    return output;
}


float4 ps_main(PixelShaderInput input) : SV_TARGET
{
    float3 wo = toCamera(input.worldPos);
    float3 n = normalize(input.normal);

#ifdef HAS_NORMAL_TEXTURE
    float3 scaledNormal = normalize(normalTexture.Sample(ModelSampler, input.tex).xyz * 2.0 - 1.0);
    n = scaledNormal;
#endif

    float2 material = getMetalnessRoughness(input.tex);
    float metalness = material.x;
    float roughness = material.y;
    float4 albedo = getAlbedo(input.tex);

    float3 finalColor =
        ambient(n, wo, albedo.rgb, roughness, metalness) +
        Lo(0, input.worldPos, n, wo, albedo.rgb, roughness, metalness) +
        Lo(1, input.worldPos, n, wo, albedo.rgb, roughness, metalness);

    return float4(finalColor, albedo.a);
}

