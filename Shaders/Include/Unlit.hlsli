#ifndef UNLIT_HLSLI
#define UNLIT_HLSLI

#include "CameraData.hlsli"
#include "TransformData.hlsli"

struct VertexInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

struct VertexOutput
{
	float4 pos : SV_POSITION;
	float4 color	: COLOR;
	float2 texcoord : TEXCOORD0;
};

[[vk::binding(0)]] ConstantBuffer<TransformData> _TransformData     : register(b0);
[[vk::binding(1)]] ConstantBuffer<CameraData>    _CameraData        : register(b1);
[[vk::binding(2)]] Texture2D                     _MainTex           : register(t0);
[[vk::binding(3)]] SamplerState                  _MainTex_Sampler   : register(s0);

#endif