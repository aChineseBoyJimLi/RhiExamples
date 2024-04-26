#ifndef RAY_TRACING_PASS_HLSL
#define RAY_TRACING_PASS_HLSL

#include "../Include/CameraData.hlsli"
#include "../Include/LightData.hlsli"

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

#endif