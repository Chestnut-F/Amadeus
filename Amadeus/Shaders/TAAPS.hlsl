#include "TAARS.hlsli"

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

Texture2D<float4> currFrame			: register(t0);
Texture2D<float4> prevFrame			: register(t1);
Texture2D<float2> velocityBuffer	: register(t2);
Texture2D<float>  depthBuffer		: register(t3);

static const float2 gInvRenderTargetSize = float2(1.0 / 1280.0, 1.0 / 720.0);

float LinearDepth(float depth)
{
	return (depth * cameraNearPlane) / (cameraFarPlane - depth * (cameraFarPlane - cameraNearPlane));
}

float3 RGB2YCoCgR(float3 rgbColor)
{
	float3 YCoCgRColor;

	YCoCgRColor.y = rgbColor.r - rgbColor.b;
	float temp = rgbColor.b + YCoCgRColor.y / 2;
	YCoCgRColor.z = rgbColor.g - temp;
	YCoCgRColor.x = temp + YCoCgRColor.z / 2;

	return YCoCgRColor;
}

float3 YCoCgR2RGB(float3 YCoCgRColor)
{
	float3 rgbColor;

	float temp = YCoCgRColor.x - YCoCgRColor.z / 2;
	rgbColor.g = YCoCgRColor.z + temp;
	rgbColor.b = temp - YCoCgRColor.y / 2;
	rgbColor.r = rgbColor.b + YCoCgRColor.y;

	return rgbColor;
}

float3 ClipAABB(float3 aabbMin, float3 aabbMax, float3 prevSample, float3 avg)
{
	float3 r = prevSample - avg;
	float3 rmax = aabbMax - avg.xyz;
	float3 rmin = aabbMin - avg.xyz;

	const float eps = 0.000001f;

	if (r.x > rmax.x + eps)
		r *= (rmax.x / r.x);
	if (r.y > rmax.y + eps)
		r *= (rmax.y / r.y);
	if (r.z > rmax.z + eps)
		r *= (rmax.z / r.z);

	if (r.x < rmin.x - eps)
		r *= (rmin.x / r.x);
	if (r.y < rmin.y - eps)
		r *= (rmin.y / r.y);
	if (r.z < rmin.z - eps)
		r *= (rmin.z / r.z);

	return avg + r;
}

struct VSOutput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

[RootSignature(Renderer_RootSig)]
float4 main(VSOutput input) : SV_TARGET
{
	int x, y;
	float4 output;

	if (bFirstFrame > 0)
	{
		return currFrame.Sample(defaultSampler, input.uv);
	}

	float2 jitteredUV = input.uv + cameraJitter;

	float2 closestOffset = float2(0.0f, 0.0f);
	float closestDepth = cameraFarPlane;
	for (y = -1; y <= 1; ++y)
	{
		for (x = -1; x <= 1; ++x)
		{
			float2 sampleOffset = float2(x, y) * gInvRenderTargetSize;
			float2 sampleUV = input.uv + sampleOffset;
			sampleUV = saturate(sampleUV);

			float NeighborhoodDepthSamp = depthBuffer.Sample(defaultSampler, sampleUV);
			NeighborhoodDepthSamp = LinearDepth(NeighborhoodDepthSamp);

			if (NeighborhoodDepthSamp < closestDepth)
			{
				closestDepth = NeighborhoodDepthSamp;
				closestOffset = float2(x, y);
			}
		}
	}
	closestOffset *= gInvRenderTargetSize;
	float2 velocity = velocityBuffer.Sample(defaultSampler, input.uv + closestOffset);

	float3 currColor = currFrame.Sample(defaultSampler, input.uv).rgb;
	currColor = RGB2YCoCgR(currColor);

	float3 prevColor = prevFrame.Sample(defaultSampler, input.uv - velocity).rgb;
	prevColor = RGB2YCoCgR(prevColor);

	static const float neighborhoodFixedSpatialWeight[9] = {
		1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 16.0f,
		1.0f / 8.0f, 1.0f / 4.0f, 1.0f / 8.0f,
		1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 16.0f };
	float TotalWeight = 0.0f;
	float3 sum = float3(0.0f, 0.0f, 0.0f);
	float3 m1 = 0.0f; // To Variance clip.
	float3 m2 = 0.0f; // To Variance clip.
	float3 neighborhood[9];
	float3 neighborMin = float3(9999999.0f, 9999999.0f, 9999999.0f);
	float3 neighborMax = float3(-99999999.0f, -99999999.0f, -99999999.0f);
	float neighborhoodFinalWeight = 0.0f;
	uint i = 0;

	for (y = -1; y <= 1; ++y)
	{
		for (x = -1; x <= 1; ++x)
		{
			float2 sampleOffset = float2(x, y) * gInvRenderTargetSize;
			float2 sampleUV = input.uv + sampleOffset;
			sampleUV = saturate(sampleUV);

			float3 NeighborhoodSamp = currFrame.Sample(defaultSampler, sampleUV).rgb;
			NeighborhoodSamp = max(NeighborhoodSamp, 0.0f);
			NeighborhoodSamp = RGB2YCoCgR(NeighborhoodSamp);

			neighborhood[i] = NeighborhoodSamp;
			neighborMin = min(neighborMin, NeighborhoodSamp);
			neighborMax = max(neighborMax, NeighborhoodSamp);
			neighborhoodFinalWeight = neighborhoodFixedSpatialWeight[i];

			m1 += NeighborhoodSamp;
			m2 += NeighborhoodSamp * NeighborhoodSamp;
			TotalWeight += neighborhoodFinalWeight;
			sum += neighborhood[i] * neighborhoodFinalWeight;

			++i;
		}
	}
	float3 Filtered = sum / TotalWeight;

	// Sharpen
	float3 highFreq = (4 * neighborhood[4]) - neighborhood[1] - neighborhood[3] - neighborhood[5] - neighborhood[7];
	float3 sharpen = Filtered + highFreq * HIGH_FREQUENCY_SCALE;
	Filtered = saturate(sharpen);

	// Variance clip.
	static const float VarianceClipGamma = 1.0f;
	float3 mu = m1 / i;
	float3 sigma = sqrt(abs(m2 / i - mu * mu));
	float3 minc = mu - VarianceClipGamma * sigma;
	float3 maxc = mu + VarianceClipGamma * sigma;

	prevColor = ClipAABB(minc, maxc, prevColor, mu);

	float weightCurr = 0.05f;
	float weightPrev = 1.0f - weightCurr;
	float RcpWeight = rcp(weightCurr + weightPrev);
	float3 color = (Filtered * weightCurr + prevColor * weightPrev) * RcpWeight;
	color = YCoCgR2RGB(color);

	output = float4(color, 1.0f);
	return output;
}