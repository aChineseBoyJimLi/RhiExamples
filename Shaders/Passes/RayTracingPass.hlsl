#ifndef RAY_TRACING_PASS_HLSL
#define RAY_TRACING_PASS_HLSL

#include "../Include/TransformData.hlsli"
#include "../Include/CameraData.hlsli"
#include "../Include/LightData.hlsli"
#include "../Include/RayTracingUtil.hlsli"

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
    float3 normal;
};

ConstantBuffer<CameraData>                  _CameraData     : register(b0);
ConstantBuffer<DirectionalLightData>        _LightData      : register(b1);
RaytracingAccelerationStructure             _AccelStructure : register(t0);
RWTexture2D<float4>                         _Output         : register(u0);

// Triangle Mesh Data
StructuredBuffer<uint>                      _Indices        : register(t1);
StructuredBuffer<float3>                    _Vertices       : register(t2);
StructuredBuffer<float2>                    _Texcoords      : register(t3);
StructuredBuffer<float3>                    _Normals        : register(t4);

// Triangle Mesh Instance data
StructuredBuffer<TransformData>             _MeshInstanceTransforms : register(t5);

uint3 GetTriangleIndices(uint triangleIndex)
{
    return uint3(_Indices[3 * triangleIndex + 0], _Indices[3 * triangleIndex + 1], _Indices[3 * triangleIndex + 2]);
}

float3 GetNormal(uint3 indices, float3 barycentrics)
{
    float3 v0 = _Normals[indices.x];
    float3 v1 = _Normals[indices.y];
    float3 v2 = _Normals[indices.z];
    return normalize(GetAttribute(barycentrics, v0, v1, v2));
}

float2 GetTexCoord(uint3 indices, float3 barycentrics)
{
    float2 v0 = _Texcoords[indices.x];
    float2 v1 = _Texcoords[indices.y];
    float2 v2 = _Texcoords[indices.z];
    return GetAttribute(barycentrics, v0, v1, v2);
}

#endif