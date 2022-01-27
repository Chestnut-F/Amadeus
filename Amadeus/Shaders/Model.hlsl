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
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
    float4 shadowCoord : TEXCOORD1;
};

cbuffer CameraConstants : register(b0)
{
    float4x4 cameraViewMatrix;
    float4x4 cameraProjectionMatrix;
    float3 cameraPosWorld;
    float cameraNearPlane;
    float cameraFarPlane;
};

cbuffer LightConstants : register(b1)
{
    float4x4 lightViewMatrix;
    float4x4 lightProjectionMatrix;
    float3 lightPosition;
    float3 lightDirection;
    float3 lightColor;
    float lightIntensity;
    float lightNearPlane;
    float lightFarPlane;
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
    float4x4 modelToShadow = mul(mul(modelMatrix, lightViewMatrix), lightProjectionMatrix);

    float4 posW = mul(float4(input.position, 1.0f), modelMatrix);
    float4 posV = mul(posW, cameraViewMatrix);
    output.positionW = posW.xyz;
    output.position = mul(posV, cameraProjectionMatrix);

    output.normal = mul(input.normal, (float3x3)modelMatrix);
    output.tangent = mul(input.tangent.xyz, (float3x3)modelMatrix);
    output.uv = input.uv;
    output.shadowCoord = mul(float4(input.position, 1.0f), modelToShadow);

	return output;
}