#include "Common.hlsli"

struct VSOutput
{
    float4 position : SV_POSITION;
};

[RootSignature(Renderer_RootSig)]
void main(VSOutput input)
{
}