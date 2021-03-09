#include "noiseSimplex.cginc"

static const float3 lightPos1 = float3(-0.3f, 0.5f, -0.3f);
static const float3 lightPos2 = float3(0.3f, 0.5f, -0.3f);
static const float3 lightPos3 = float3(0, 0.5f, 0.3f);

// A constant buffer that stores the three light colors.
cbuffer LightConstantBuffer : register(b0)
{
    float4 lightColor1;
    float4 lightColor2;
    float4 lightColor3;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR0;
    float3 normal : NORMAL;
    float3 worldPos : WORLD_POSITION;
};

float sqr(float x)
{
    return pow(x, 69);
}

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 appliedLight = float3(1, 1, 1) * 0.56f;
    
    float3 vectorToLight = normalize(lightPos1 - input.worldPos);
    float3 diffuseLightIntensity = max(dot(vectorToLight, input.normal), 0);
    float attenuation1 = 1 / (1 + sqr(distance(lightPos1, input.worldPos.xyz)));
    float3 diffuseLight = diffuseLightIntensity *
        lightColor1.w * lightColor1.xyz * attenuation1;

    appliedLight += diffuseLight;

    float3 vectorToLight2 = normalize(lightPos2 - input.worldPos);
    float3 diffuseLightIntensity2 = max(dot(vectorToLight2, input.normal), 0);
    float attenuation2 = 1 / (1 + sqr(distance(lightPos2, input.worldPos.xyz)));
    float3 diffuseLight2 = diffuseLightIntensity2 *
        lightColor2.w * lightColor2.xyz * attenuation2;

    appliedLight += diffuseLight2;

    float3 vectorToLight3 = normalize(lightPos3 - input.worldPos);
    float3 diffuseLightIntensity3 = max(dot(vectorToLight3, input.normal), 0);
    float attenuation3 = 1 / (1 + sqr(distance(lightPos3, input.worldPos.xyz)));
    float3 diffuseLight3 = diffuseLightIntensity3 *
        lightColor3.w * lightColor3.xyz * attenuation3;

    appliedLight += diffuseLight3;

    //float3 finalColor = input.color * appliedLight;
    float3 finalColor = (0.5f + snoise(input.worldPos * 10) / 4) * appliedLight;

    return float4(finalColor, 1.0f);
}
