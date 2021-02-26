// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

PixelShaderInput main(uint vI : SV_VERTEXID)
{
    PixelShaderInput result;

    result.texcoord = float2(vI & 1,vI >> 1);
    result.pos = float4((result.texcoord.x - 0.5f) * 2, -(result.texcoord.y - 0.5f) * 2, 0, 1);

    return result;
}