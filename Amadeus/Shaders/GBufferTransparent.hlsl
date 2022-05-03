#include "Common.hlsli"

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

cbuffer MaterialConstants : register(b2)
{
    float4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    float3 emissiveFactor;
};

SamplerState modelSampler : register(s0);

Texture2D<float4> baseColorTexture          : register(t0);
Texture2D<float4> metallicRoughnessTexture  : register(t1);
Texture2D<float4> occlusionTexture          : register(t2);
Texture2D<float4> emissiveTexture           : register(t3);
Texture2D<float4> normalTexture             : register(t4);

Texture2D<float2> brdflutTexture            : register(t7);
TextureCube       prefilteredMapTexture     : register(t8);

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
    float4 shadowCoord : TEXCOORD1;
    float4 curCoord : TEXCOORD2;
    float4 prevCoord : TEXCOORD3;
};

struct GBuffer
{
    float4 normal                       : SV_TARGET0;
    float4 baseColor                    : SV_TARGET1;
    float4 metallicSpecularRoughness    : SV_TARGET2;
    float2 velocity                     : SV_TARGET3;
};

float2 GetVelocity(float4 PrevCoord, float4 CurCoord)
{
    PrevCoord.xyz = PrevCoord.xyz / PrevCoord.w;
    float2 prevTexCoord = float2((PrevCoord.x + 1) * 0.5, (-PrevCoord.y + 1) * 0.5);

    CurCoord.xyz = CurCoord.xyz / CurCoord.w;
    float2 curTexCoord = float2((CurCoord.x + 1) * 0.5, (-CurCoord.y + 1) * 0.5);

    prevTexCoord.x = (prevTexCoord.x > 1.0 || prevTexCoord.x < 0.0) ? curTexCoord.x : prevTexCoord.x;
    prevTexCoord.y = (prevTexCoord.y > 1.0 || prevTexCoord.y < 0.0) ? curTexCoord.y : prevTexCoord.y;

    float2 velocity = curTexCoord - prevTexCoord;
    return velocity;
}

float3 PrefilteredReflection(float3 R, float roughness)
{
    const float MAX_REFLECTION_LOD = 10.0; // todo: param/const
    float lod = roughness * MAX_REFLECTION_LOD;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    float3 a = prefilteredMapTexture.SampleLevel(defaultSampler, R, lodf).rgb;
    float3 b = prefilteredMapTexture.SampleLevel(defaultSampler, R, lodc).rgb;
    return lerp(a, b, lod - lodf);
}

float3 PrefilteredIrradiance(float3 N)
{
    const float MAX_REFLECTION_LOD = 10.0;
    float3 irradiance = prefilteredMapTexture.SampleLevel(defaultSampler, N, MAX_REFLECTION_LOD).rgb;
    return irradiance;
}

float3 FresnelSchlick(float3 F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

[earlydepthstencil]
[RootSignature(Renderer_RootSig)]
GBuffer main(VSOutput input) : SV_TARGET
{
    GBuffer output;

    output.normal = float4(normalize(input.normal) * 0.5 + 0.5, 1.0);

    float3 N = input.normal;
    float3 V = normalize(cameraPosWorld - input.positionW);
    float3 R = 2 * dot(V, N) * N - V;
    float NoV = max(dot(N, V), 0.0);

    float3 albedo = baseColorFactor.rgb * baseColorTexture.Sample(defaultSampler, input.uv).rgb;
    float metallic = metallicFactor * metallicRoughnessTexture.Sample(defaultSampler, input.uv).b;
    float roughness = roughnessFactor * metallicRoughnessTexture.Sample(defaultSampler, input.uv).g;
    float3 emissive = emissiveFactor * emissiveTexture.Sample(defaultSampler, input.uv).rgb;
    float occlusion = occlusionStrength * occlusionTexture.Sample(defaultSampler, input.uv).r;

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, albedo, metallic);
    float3 F = FresnelSchlick(F0, NoV);
    float3 kd = lerp(1.0 - F, 0.0, metallic);

    float2 brdf = brdflutTexture.Sample(defaultSampler, float2(NoV, roughness)).rg;
    float3 reflection = PrefilteredReflection(R, roughness).rgb;
    float3 irradiance = PrefilteredIrradiance(N).rgb;

    float3 diffuse = occlusion * kd * irradiance * albedo;
    float3 specular = reflection * (F * brdf.x + brdf.y);
    float3 ambient = emissive + diffuse + specular;

    output.baseColor = occlusion * float4(ambient, 1.0);
    output.metallicSpecularRoughness = float4(1.0, 1.0, 1.0, 1.0);
    output.velocity = GetVelocity(input.prevCoord, input.curCoord);

    return output;
}