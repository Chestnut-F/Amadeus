#include "ScreenSpaceRS.hlsli"

Texture2D<float4> positionTexture	: register(t0);
Texture2D<float3> normalTexture		: register(t1);
Texture2D<float3> ssaoNoise			: register(t2);

cbuffer CameraConstants : register(b0)
{
    float4x4 cameraViewMatrix;
    float4x4 cameraProjectionMatrix;
    float3 cameraPosWorld;
    float cameraNearPlane;
    float cameraFarPlane;
};

cbuffer SSAOKernel : register(b1)
{
    float3 gSSAOKernel[SSAO_KERNEL_SIZE];
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

[RootSignature(Renderer_RootSig)]
float main(VSOutput input) : SV_TARGET
{
	float3 pixPos = positionTexture.Sample(defaultSampler, input.uv).rgb;
	float3 pixNormal = normalize(normalTexture.Sample(defaultSampler, input.uv).rgb * 2.0 - 1.0);

	float3 randomVec = (ssaoNoise.Sample(noiseSampler, input.uv).xyz + 1.0) / 2.0;

	float3 tangent = normalize(randomVec - pixNormal * dot(randomVec, pixNormal)); // Gram¨CSchmidt process
	float3 bitangent = normalize(cross(tangent, pixNormal));
	float3x3 TBN = float3x3(tangent, bitangent, pixNormal);

	float occlusion = 0.0f;
	for (int i = 0; i < SSAO_KERNEL_SIZE; i++)
	{
		float3 samplePos = mul(gSSAOKernel[i].xyz, TBN);
		samplePos = pixPos + samplePos * SSAO_RADIUS;

		float4 offset = float4(samplePos, 1.0f);
		offset = mul(offset, cameraProjectionMatrix);
		offset.xyz /= offset.w;
		offset.xy = float2(offset.x * 0.5f + 0.5f, 1.0f - (offset.y * 0.5f + 0.5f));

		float sampleDepth = positionTexture.Sample(defaultSampler, offset.xy).z;

		float rangeCheck = smoothstep(0.0f, 1.0f, SSAO_RADIUS / abs(pixPos.z - sampleDepth));
		occlusion += (samplePos.z >= sampleDepth ? 1.0f : 0.0f) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));

	return occlusion;
}