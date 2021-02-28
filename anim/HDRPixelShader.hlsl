Texture2D shaderTexture;
SamplerState samplerState;

cbuffer averageLogBrightnessConstantBuffer : register(b0)
{
    float averageLogBrightness;
    float minL;
    float maxL;
    float dummy;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

static const float A = 0.1;  // Shoulder Strength
static const float B = 0.50; // Linear Strength
static const float C = 0.1;  // Linear Angle
static const float D = 0.20; // Toe Strength
static const float E = 0.02; // Toe Numerator
static const float F = 0.30; // Toe Denominator
                             // Note: E/F = Toe Angle
static const float W = 11.2; // Linear White Point Value

float3 uncharted2Tonemap(float3 x)
{
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float keyValue(float l)
{
    return 1.03 - 2 / (2 + log10(l + 1));
}

float exposure()
{
    float l = exp(averageLogBrightness) - 1;
    //return keyValue(l) / (clamp(l, minL, maxL));
    return keyValue(l) / l;
}

float3 tonemapFilmic(float3 color)
{
    float E = exposure();
    float3 curr = uncharted2Tonemap(E * color);
    float3 whiteScale = 1.0f / uncharted2Tonemap(W);
    return curr * whiteScale;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    float4 textureColor = shaderTexture.Sample(samplerState, input.texcoord);
    //return float4(tonemapFilmic(textureColor.rgb), 1);
    return float4(pow(tonemapFilmic(textureColor.rgb), 1 / 2.2), 1);
}