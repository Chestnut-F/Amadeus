#define Renderer_RootSig \
    "RootFlags(0), " \
    "CBV(b0), " \
    "DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_BORDER," \
        "addressV = TEXTURE_ADDRESS_BORDER," \
        "addressW = TEXTURE_ADDRESS_BORDER," \
        "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
        "filter = FILTER_MIN_MAG_MIP_POINT) " \

// Common (static) samplers
SamplerState defaultSampler : register(s0);

static const float3 gTexCoords[36] =
{
    float3(-1000.0f,  1000.0f, -1000.0f),
    float3(-1000.0f, -1000.0f, -1000.0f),
    float3(1000.0f, -1000.0f, -1000.0f),
    float3(1000.0f, -1000.0f, -1000.0f),
    float3(1000.0f,  1000.0f, -1000.0f),
    float3(-1000.0f,  1000.0f, -1000.0f),

    float3(-1000.0f, -1000.0f,  1000.0f),
    float3(-1000.0f, -1000.0f, -1000.0f),
    float3(-1000.0f,  1000.0f, -1000.0f),
    float3(-1000.0f,  1000.0f, -1000.0f),
    float3(-1000.0f,  1000.0f,  1000.0f),
    float3(-1000.0f, -1000.0f,  1000.0f),

    float3(1000.0f, -1000.0f, -1000.0f),
    float3(1000.0f, -1000.0f,  1000.0f),
    float3(1000.0f,  1000.0f,  1000.0f),
    float3(1000.0f,  1000.0f,  1000.0f),
    float3(1000.0f,  1000.0f, -1000.0f),
    float3(1000.0f, -1000.0f, -1000.0f),

    float3(-1000.0f, -1000.0f,  1000.0f),
    float3(-1000.0f,  1000.0f,  1000.0f),
    float3(1000.0f,  1000.0f,  1000.0f),
    float3(1000.0f,  1000.0f,  1000.0f),
    float3(1000.0f, -1000.0f,  1000.0f),
    float3(-1000.0f, -1000.0f,  1000.0f),

    float3(-1000.0f,  1000.0f, -1000.0f),
    float3(1000.0f,  1000.0f, -1000.0f),
    float3(1000.0f,  1000.0f,  1000.0f),
    float3(1000.0f,  1000.0f,  1000.0f),
    float3(-1000.0f,  1000.0f,  1000.0f),
    float3(-1000.0f,  1000.0f, -1000.0f),

    float3(-1000.0f, -1000.0f, -1000.0f),
    float3(-1000.0f, -1000.0f,  1000.0f),
    float3(1000.0f, -1000.0f, -1000.0f),
    float3(1000.0f, -1000.0f, -1000.0f),
    float3(-1000.0f, -1000.0f,  1000.0f),
    float3(1000.0f, -1000.0f,  1000.0f)
};