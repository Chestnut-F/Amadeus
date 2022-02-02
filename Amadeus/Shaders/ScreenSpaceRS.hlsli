#define Renderer_RootSig \
    "RootFlags(0), " \
    "CBV(b0), " \
    "CBV(b1), " \
    "DescriptorTable(SRV(t0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t1, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t2, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_BORDER," \
        "addressV = TEXTURE_ADDRESS_BORDER," \
        "addressW = TEXTURE_ADDRESS_BORDER," \
        "borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK," \
        "filter = FILTER_MIN_MAG_MIP_POINT), " \
    "StaticSampler(s1," \
        "addressU = TEXTURE_ADDRESS_WRAP," \
        "addressV = TEXTURE_ADDRESS_WRAP," \
        "addressW = TEXTURE_ADDRESS_WRAP," \
        "filter = FILTER_MIN_MAG_MIP_POINT)"

// Common (static) samplers
SamplerState defaultSampler : register(s0);
SamplerState noiseSampler : register(s1);

#define SSAO_KERNEL_SIZE 64
#define SSAO_RADIUS 0.5