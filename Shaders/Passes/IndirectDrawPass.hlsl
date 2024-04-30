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
    float           Padding0;
    float3          Max; // AABB
    float           Padding1;
    uint			MatIndex;
    uint			Padding2;
    uint			Padding3;
    uint			Padding4;
    uint			Padding[20];
};

struct MaterialData
{
    float4 Color;
    float  Smooth;
    uint   TexIndex;
    float  Padding0;
    float  Padding1;
};

ConstantBuffer<CameraData>                  _CameraData : register(b0);
ConstantBuffer<DirectionalLightData>        _LightData : register(b1);
ConstantBuffer<InstanceData>                _InstanceData : register(b2);
Texture2D								    _MainTex[]		: register(t0, space1); // unbounded texture array
StructuredBuffer<MaterialData>			    _MaterialData	: register(t0);
SamplerState							    _MainTex_Sampler: register(s0); 

#endif