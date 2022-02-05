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
    float3 positionW : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

cbuffer CameraConstants : register(b0)
{
    float4x4 cameraViewMatrix;
    float4x4 cameraProjectionMatrix;
    float4x4 cameraUnjitteredProjectionMatrix;
    float3 cameraPosWorld;
    float cameraNearPlane;
    float cameraFarPlane;
    float2 cameraJitter;
    uint bFirstFrame;
    float4x4 cameraPrevViewProjectionMatrix;
};

cbuffer ModelConstants : register(b2)
{
    float4x4 modelMatrix;
};

[RootSignature(Renderer_RootSig)]
VSOutput main(VSInput input)
{
    VSOutput output;

    float4x4 modelViewMatrix = mul(modelMatrix, cameraViewMatrix);

    float4 posW = mul(float4(input.position, 1.0f), modelViewMatrix);
    output.positionW = posW.xyz;
    output.position = mul(posW, cameraProjectionMatrix);

    output.normal = mul(input.normal, (float3x3)modelViewMatrix);
    output.uv = input.uv;

    return output;
}