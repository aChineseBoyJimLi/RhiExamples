#ifndef UNLIT_PASS_HLSL
#define UNLIT_PASS_HLSL

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
	uint			MatIndex;
	uint			Padding0;
	uint			Padding1;
	uint			Padding2;
};

struct MaterialData
{
	float4 Color;
	float  Smooth;
	uint   TexIndex;
	float  Padding0;
	float  Padding1;
};

ConstantBuffer<CameraData>				_CameraData		: register(b0);
ConstantBuffer<DirectionalLightData>	_LightData		: register(b1);
StructuredBuffer<InstanceData>			_InstanceData	: register(t0);
StructuredBuffer<MaterialData>			_MaterialData	: register(t1);
SamplerState							_MainTex_Sampler: register(s0);
Texture2D								_MainTex[]		: register(t0, space1); // occupy registers t0, t1, ... in space1
#endif