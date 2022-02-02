#include "Common.hlsli"

cbuffer MaterialConstants : register(b2)
{
    float4 baseColorFactor;
};

SamplerState modelSampler : register(s0);

Texture2D<float4> baseColorTexture          : register(t0);
Texture2D<float4> metallicRoughnessTexture  : register(t1);
Texture2D<float4> occlusionTexture          : register(t2);
Texture2D<float4> emissiveTexture           : register(t3);
Texture2D<float4> normalTexture             : register(t4);

Texture2D<float>  shadowMap                 : register(t5);
Texture2D<float>  ssaoTexture               : register(t6);

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
    float4 shadowCoord : TEXCOORD1;
};

struct GBuffer
{
    float4 normal                       : SV_TARGET0;
    float4 baseColor                    : SV_TARGET1;
    float4 metallicSpecularRoughness    : SV_TARGET2;
    float4 velocity                     : SV_TARGET3;
};

float GetDirectionalShadow(float4 ShadowCoord)
{
    ShadowCoord.xyz = ShadowCoord.xyz / ShadowCoord.w;
    float2 texCoord = float2((ShadowCoord.x + 1) * 0.5, (-ShadowCoord.y + 1) * 0.5);
    const float Dilation = 2.0;
    const float ShadowTexelSize = 1.0 / 2048.0;
    float d1 = Dilation * ShadowTexelSize * 0.125;
    float d2 = Dilation * ShadowTexelSize * 0.875;
    float d3 = Dilation * ShadowTexelSize * 0.625;
    float d4 = Dilation * ShadowTexelSize * 0.375;

    float depth = 0;
    depth += shadowMap.Sample(defaultSampler, texCoord) < ShadowCoord.z ? 0.4f : 2.0f;
    depth += shadowMap.Sample(defaultSampler, texCoord + float2(-d2, d1)) < ShadowCoord.z ? 0.2f : 1.0f;
    depth += shadowMap.Sample(defaultSampler, texCoord + float2(-d1, -d2)) < ShadowCoord.z ? 0.2f : 1.0f;
    depth += shadowMap.Sample(defaultSampler, texCoord + float2(d2, -d1)) < ShadowCoord.z ? 0.2f : 1.0f;
    depth += shadowMap.Sample(defaultSampler, texCoord + float2(d1, d2)) < ShadowCoord.z ? 0.2f : 1.0f;
    depth += shadowMap.Sample(defaultSampler, texCoord + float2(-d4, d3)) < ShadowCoord.z ? 0.2f : 1.0f;
    depth += shadowMap.Sample(defaultSampler, texCoord + float2(-d3, -d4)) < ShadowCoord.z ? 0.2f : 1.0f;
    depth += shadowMap.Sample(defaultSampler, texCoord + float2(d4, -d3)) < ShadowCoord.z ? 0.2f : 1.0f;
    depth += shadowMap.Sample(defaultSampler, texCoord + float2(d3, d4)) < ShadowCoord.z ? 0.2f : 1.0f;
    float result = depth / 10.0f;

    return result * result;
}

float GetAO(float4 position)
{
    uint2 pixelPos = uint2(position.xy);
    float ao = ssaoTexture[pixelPos];
    return ao;
}

[earlydepthstencil]
[RootSignature(Renderer_RootSig)]
GBuffer main(VSOutput input) : SV_TARGET
{
    GBuffer output;

    output.normal = float4(normalize(input.normal) * 0.5 + 0.5, 1.0);

    float shadow = GetDirectionalShadow(input.shadowCoord);
    float occlusion = GetAO(input.position);

    output.baseColor = occlusion * shadow * baseColorTexture.Sample(modelSampler, input.uv) * baseColorFactor;
    output.metallicSpecularRoughness = float4(1.0, 1.0, 1.0, 1.0);
    output.velocity = float4(1.0, 1.0, 1.0, 1.0);

	return output;
}