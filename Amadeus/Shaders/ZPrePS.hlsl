#include "Common.hlsli"

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct ZPre
{
    float4 position : SV_TARGET0;
    float3 normal   : SV_TARGET1;
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

float linearDepth(float depth)
{
    float z = depth * 2.0f - 1.0f;
    return (2.0f * cameraNearPlane * cameraFarPlane) / (cameraFarPlane + cameraNearPlane - z * (cameraFarPlane - cameraNearPlane));
}

[RootSignature(Renderer_RootSig)]
ZPre main(VSOutput input)
{
    ZPre output;

    output.position = float4(input.positionW, linearDepth(input.position.z));
    output.normal = normalize(input.normal) * 0.5 + 0.5;

    return output;
}