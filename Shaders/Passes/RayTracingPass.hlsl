#ifndef RAY_TRACING_PASS_HLSL
#define RAY_TRACING_PASS_HLSL

#define MAX_RAY_RECURSION_DEPTH 3 // ~ primary rays + reflections + shadow rays from reflected geometry.
#define RADIANCE_RAY_MISS_SHADER_INDEX 0
#define SHADOW_RAY_MISS_SHADER_INDEX 1

#include "../Include/TransformData.hlsli"
#include "../Include/CameraData.hlsli"
#include "../Include/LightData.hlsli"
#include "../Include/RayTracingUtil.hlsli"
#include "../Include/Common.hlsli"

struct Payload
{
    float3 HitValue;
    uint   Depth;
};

struct ShadowPayload
{
    bool Hit;
};

struct TrianglePrimitiveAttributes 
{
    float2 UV;
};

struct ProceduralPrimitiveAttributes
{
    float3 WorldNormal;
};

struct InstanceData
{
    TransformData	Transform;
    uint			MatIndex;
    uint			Padding0;
    uint			Padding1;
    uint			Padding2;
};

struct MaterialData
{
    float4 Color;
    uint   TextureIndex;
    uint   IsLambertian; // 0: Mirror reflection, 1: Lambertian
    uint   Padding0;
    uint   Padding1;
};

ConstantBuffer<CameraData>                  _CameraData     : register(b0);
ConstantBuffer<DirectionalLightData>        _LightData      : register(b1);
RaytracingAccelerationStructure             _AccelStructure : register(t0);
RWTexture2D<float4>                         _Output         : register(u0);

// Triangle Mesh Data
StructuredBuffer<uint>                      _Indices        : register(t1);
StructuredBuffer<float2>                    _Texcoords      : register(t2);
StructuredBuffer<float4>                    _Normals        : register(t3);

// Triangle Mesh Instance data
StructuredBuffer<InstanceData>              _InstanceData : register(t4);
StructuredBuffer<MaterialData>              _MaterialData : register(t5);

uint3 GetTriangleIndices(uint triangleIndex)
{
    return uint3(_Indices[3 * triangleIndex + 0], _Indices[3 * triangleIndex + 1], _Indices[3 * triangleIndex + 2]);
}

float3 GetNormal(uint3 indices, float3 barycentrics)
{
    float3 v0 = _Normals[indices.x].xyz;
    float3 v1 = _Normals[indices.y].xyz;
    float3 v2 = _Normals[indices.z].xyz;
    return normalize(GetAttribute(barycentrics, v0, v1, v2));
}

float2 GetTexCoord(uint3 indices, float3 barycentrics)
{
    float2 v0 = _Texcoords[indices.x];
    float2 v1 = _Texcoords[indices.y];
    float2 v2 = _Texcoords[indices.z];
    return GetAttribute(barycentrics, v0, v1, v2);
}

float4 TraceRadianceRay(in float3 origin, in float3 direction, in uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return float4(0, 0, 0, 0);
    }
    
    RayDesc rayDesc;
    rayDesc.Origin = origin;
    rayDesc.Direction = direction;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    Payload payload;
    payload.HitValue = float3(0, 0, 0);
    payload.Depth = currentRayRecursionDepth + 1;
    
    TraceRay(_AccelStructure
        , RAY_FLAG_FORCE_OPAQUE
        , 0xff, 0, 0
        , RADIANCE_RAY_MISS_SHADER_INDEX
        , rayDesc, payload);

    return float4(payload.HitValue, 1);
}

bool TraceShadowRay(in float3 origin, in float3 direction, in uint currentRayRecursionDepth)
{
    if (currentRayRecursionDepth >= MAX_RAY_RECURSION_DEPTH)
    {
        return false;
    }
    
    RayDesc rayDesc;
    rayDesc.Origin = origin;
    rayDesc.Direction = direction;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    ShadowPayload payload;
    payload.Hit = true;

    TraceRay(_AccelStructure
        , RAY_FLAG_CULL_BACK_FACING_TRIANGLES
        | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
        | RAY_FLAG_FORCE_OPAQUE             // ~skip any hit shaders
        | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER  // ~skip closest hit shaders,
        , 0xff, 0, 0
        , SHADOW_RAY_MISS_SHADER_INDEX
        , rayDesc, payload);

    return payload.Hit;
}

float3 SufaceLighting(in float3 hitPositionWS, in float3 normalWS, in MaterialData material, in uint currentRayRecursionDepth)
{
    float3 incident = normalize(-_LightData.LightDirection);
    bool inShadow = TraceShadowRay(hitPositionWS, incident, currentRayRecursionDepth);
    float shadow = inShadow ? 0.5f : 1.0f;

    float NdotL = saturate(dot(normalWS, incident));

    if (material.IsLambertian == 1)
    {
        return material.Color.xyz * _LightData.LightIntensity * _LightData.LightColor * NdotL * shadow;
    }
    else
    {
        float reflectance = 0.3f;
        float4 reflectedColor = TraceRadianceRay(hitPositionWS, reflect(WorldRayDirection(), normalWS), currentRayRecursionDepth);
        float3 lightingColor = _LightData.LightIntensity * _LightData.LightColor * NdotL + reflectance * reflectedColor.xyz;
        return material.Color.xyz * lightingColor * shadow;
    }
}

#endif