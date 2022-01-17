#include "Common.hlsli"

static const float2 TexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

[RootSignature(Renderer_RootSig)]
VSOutput main(uint vertexIndex : SV_VertexID)
{
    VSOutput output;
    output.uv = TexCoords[vertexIndex];
    output.position = float4(output.uv.x * 2.0f - 1.0f, 1.0f - output.uv.y * 2.0f, 0.0f, 1.0f);
    return output;
}