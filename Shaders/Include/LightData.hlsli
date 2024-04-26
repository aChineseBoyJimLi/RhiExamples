#ifndef LIGHT_DATA_HLSLI
#define LIGHT_DATA_HLSLI

struct DirectionalLightData
{
    float4x4 WorldToLight;
    float4x4 LightToWorld;
    float4x4 LightProjection;
    float3   LightDirection;
    float    LightIntensity;
    float3   LightColor;
};

#endif // LIGHT_DATA_HLSLI