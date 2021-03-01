static const float3 lightPos1 = float3(1.0f, 0.0f, 0.0f);
static const float3 lightPos2 = float3(1.0f, 0.0f, 1.0f);
static const float3 lightPos3 = float3(1.0f, 1.0f, 1.0f);

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

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 appliedLight = float3(1.0f, 1.0f, 1.0f);
    
    float3 vectorToLight = normalize(lightPos1 - input.worldPos);
    float3 diffuseLightIntensity = max(dot(vectorToLight, input.normal), 0);
    float3 diffuseLight = diffuseLightIntensity * lightColor1.w * lightColor1.xyz;

    appliedLight += diffuseLight;

    float3 vectorToLight2 = normalize(lightPos2 - input.worldPos);
    float3 diffuseLightIntensity2 = max(dot(vectorToLight2, input.normal), 0);
    float3 diffuseLight2 = diffuseLightIntensity2 * lightColor2.w * lightColor2.xyz;

    appliedLight += diffuseLight2;

    float3 vectorToLight3 = normalize(lightPos3 - input.worldPos);
    float3 diffuseLightIntensity3 = max(dot(vectorToLight3, input.normal), 0);
    float3 diffuseLight3 = diffuseLightIntensity3 * lightColor3.w * lightColor3.xyz;

    appliedLight += diffuseLight3;

    float3 finalColor = input.color * appliedLight;

    return float4(finalColor, 1.0f);
}
