#include "Common.hlsli"

struct VSInput
{
    float3 position : POSITION0;
    float3 normal : NORMAL0;
    float4 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 positionWorld : POSITION0;
    float3 normal : NORMAL0;
    float3 tangent : TANGENT0;
    float2 uv : TEXCOORD0;
};

cbuffer GlobalConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

cbuffer MeshConstants : register(b1)
{
    float4x4 modelMatrix;
}

[RootSignature(Renderer_RootSig)]
VSOutput main(VSInput input)
{
    VSOutput output;

    float4x4 modelViewMatrix = mul(modelMatrix, viewMatrix);

    float4 posW = mul(float4(input.position, 1.0f), modelViewMatrix);
    output.positionWorld = posW.xyz;
    output.position = mul(posW, projectionMatrix);

    output.normal = mul(input.normal, (float3x3)modelViewMatrix);
    output.tangent = mul(input.tangent.xyz, (float3x3)modelViewMatrix);
    output.uv = input.uv;

	return output;
}