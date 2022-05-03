#include "SkyboxRS.hlsli"

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

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 uvw : TEXCOORD;
};

[RootSignature(Renderer_RootSig)]
VSOutput main(uint vertexIndex : SV_VertexID)
{
	VSOutput output;
	output.uvw = gTexCoords[vertexIndex];
	float3 pos = gTexCoords[vertexIndex] + cameraPosWorld;
    float4x4 viewProjectionMatrix = mul(cameraViewMatrix, cameraProjectionMatrix);
	output.pos = mul(float4(pos, 1.0), viewProjectionMatrix);
	return output;
}