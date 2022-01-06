#define Renderer_RootSig \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
    "CBV(b0), " \
    "CBV(b1), " \
    "CBV(b2, visibility = SHADER_VISIBILITY_VERTEX), " \
    "CBV(b2, visibility = SHADER_VISIBILITY_PIXEL), " \
    "DescriptorTable(SRV(t0, numDescriptors = 5), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t5, numDescriptors = 10), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(Sampler(s0, numDescriptors = 1), visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s1, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s2, visibility = SHADER_VISIBILITY_PIXEL," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "comparisonFunc = COMPARISON_GREATER_EQUAL," \
        "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)," \
    "StaticSampler(s3, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)"

// Common (static) samplers
SamplerState defaultSampler : register(s1);
SamplerComparisonState shadowSampler : register(s2);
SamplerState cubeMapSampler : register(s3);