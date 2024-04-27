#ifndef INDIRECT_DRAW_PASS_HLSL
#define INDIRECT_DRAW_PASS_HLSL

#include "../Include/CameraData.hlsli"
#include "../Include/TransformData.hlsli"
#include "../Include/LightData.hlsli"

struct VertexInput
{
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float2 TexCoord : TEXCOORD;
};

struct VertexOutput
{
    float4 Position      : SV_POSITION;
    float3 WorldNormal   : NORMAL;
    float2 TexCoord		 : TEXCOORD0;
    nointerpolation uint MatIndex  : MATINDEX;
};

struct InstanceData
{
    TransformData	Transform;
    float3          Min; // AABB
    float3          Max; // AABB
    float           Padding0;
    float           Padding1;
    uint			MatIndex;
    uint			Padding2;
    uint			Padding3;
    uint			Padding4;
};

#endif