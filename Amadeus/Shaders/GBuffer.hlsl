#include "Common.hlsli"

cbuffer MaterialConstants : register(b1)
{
    float4 baseColorFactor;
};

SamplerState modelSampler : register(s0);

Texture2D<float4> baseColorTexture          : register(t0);
Texture2D<float4> metallicRoughnessTexture  : register(t1);
Texture2D<float4> occlusionTexture          : register(t2);
Texture2D<float4> emissiveTexture           : register(t3);
Texture2D<float4> normalTexture             : register(t4);

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct GBuffer
{
    float4 normal                       : SV_TARGET0;
    float4 baseColor                    : SV_TARGET1;
    float4 metallicSpecularRoughness    : SV_TARGET2;
    float4 velocity                     : SV_TARGET3;
};

[RootSignature(Renderer_RootSig)]
GBuffer main(VSOutput input) : SV_TARGET
{
    GBuffer output;

    output.normal = float4(normalize(input.normal) * 0.5 + 0.5, 1.0);
    output.baseColor = baseColorTexture.Sample(modelSampler, input.uv) * baseColorFactor;

	return output;
}