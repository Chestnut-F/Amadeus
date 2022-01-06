#include "Common.hlsli"

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

SamplerState modelSampler : register(s0);
Texture2D<float3> texBaseColor : register(t0);

[RootSignature(Renderer_RootSig)]
float4 main(VSOutput input) : SV_TARGET
{
    float gamma = 2.2;
    float4 pixelColor;
    pixelColor.rgb = pow(texBaseColor.Sample(modelSampler, input.uv).rgb, 1.0 / gamma);
    return pixelColor;
}