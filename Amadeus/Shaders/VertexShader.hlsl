#include "Common.hlsli"

struct VSInput
{
    float3 position : POSITION0;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer CameraConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float3 cameraPosWorld;
    float nearPlane;
    float farPlane;
};

[RootSignature(Renderer_RootSig)]
VSOutput main(VSInput input)
{
    VSOutput output;

    float4 pos = mul(float4(input.position, 1.0f), viewMatrix);
    output.position = mul(pos, projectionMatrix);
    output.color = input.color;

    return output;
}