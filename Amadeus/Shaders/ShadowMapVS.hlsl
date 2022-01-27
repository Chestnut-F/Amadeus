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
};

cbuffer LightConstants : register(b1)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float3 position;
    float3 direction;
    float3 color;
    float intensity;
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

    float4x4 modelViewMatrix = mul(modelMatrix, viewMatrix);
    float4x4 modelViewProjectionMatrix = mul(modelViewMatrix, projectionMatrix);
    output.position = mul(float4(input.position, 1.0f), modelViewProjectionMatrix);

	return output;
}