#include "Common.hlsli"

cbuffer MaterialConstants : register(b1)
{
    float4 baseColorFactor;
};

Texture2D<float4> baseColorTexture          : register(t0);
Texture2D<float4> metallicRoughnessTexture  : register(t1);
Texture2D<float4> occlusionTexture          : register(t2);
Texture2D<float4> emissiveTexture           : register(t3);
Texture2D<float4> normalTexture             : register(t4);

SamplerState baseColorSampler               : register(s0);
SamplerState metallicRoughnessSampler       : register(s1);
SamplerState occlusionSampler               : register(s2);
SamplerState emissiveSampler                : register(s3);
SamplerState normalSampler                  : register(s4);

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 positionWorld : POSITION0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

struct GBuffer
{
    float3 normal : SV_TARGET0;
    float4 albedo : SV_TARGET1;
};

[RootSignature(Renderer_RootSig)]
GBuffer main(VSOutput input) : SV_TARGET
{
    GBuffer output;

    output.normal = float4(normalize(input.normal) * 0.5 + 0.5, 1.0);
    output.albedo = baseColorTexture.Sample(baseColorSampler, input.uv) * baseColorFactor;

	return output;
}