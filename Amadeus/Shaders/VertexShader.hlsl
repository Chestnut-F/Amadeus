#include "Common.hlsli"

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer CameraConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float3 cameraPosWorld;
    float nearPlane;
    float farPlane;
};

cbuffer ModelConstants : register(b2)
{
    float4x4 modelMatrix;
};

[RootSignature(Renderer_RootSig)]
VSOutput main(VSInput input)
{
    VSOutput output;

    float4 posW = mul(float4(input.position, 1.0f), modelMatrix);
    float4 posV = mul(posW, viewMatrix);
    output.position = mul(posV, projectionMatrix);
    output.uv = input.uv;

    return output;
}