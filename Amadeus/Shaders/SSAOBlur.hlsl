#include "ScreenSpaceRS.hlsli"

Texture2D ssaoTexture : register(t0);

struct VSOutput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

[RootSignature(Renderer_RootSig)]
float main(VSOutput input) : SV_TARGET
{
	const int blurRange = 5;
	int n = 0;
	int2 texDim;
	ssaoTexture.GetDimensions(texDim.x, texDim.y);
	float2 texelSize = 1.0 / (float2)texDim;
	float result = 0.0;
	for (int x = -blurRange; x <= blurRange; x++)
	{
		for (int y = -blurRange; y <= blurRange; y++)
		{
			float2 offset = float2(float(x), float(y)) * texelSize;
			result += ssaoTexture.Sample(defaultSampler, input.uv + offset).r;
			n++;
		}
	}
	return result / (float(n));
}