#ifndef UNLIT_PASS_HLSL
#define UNLIT_PASS_HLSL

#include "../Include/CameraData.hlsli"
#include "../Include/TransformData.hlsli"

struct VertexInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
    float2 texcoord : TEXCOORD;
};

struct VertexOutput
{
	float4 pos      : SV_POSITION;
	float4 color	: COLOR;
	float2 texcoord : TEXCOORD0;
};

ConstantBuffer<TransformData> _TransformData     : register(b0);
ConstantBuffer<CameraData>    _CameraData        : register(b1);
Texture2D                     _MainTex           : register(t0);
SamplerState                  _MainTex_Sampler   : register(s0);

#endif