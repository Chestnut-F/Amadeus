#include "Common.hlsli"

struct VSOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

[RootSignature(Renderer_RootSig)]
float4 main(VSOutput input) : SV_TARGET
{
    return input.color;
}