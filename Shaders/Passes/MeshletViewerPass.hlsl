#ifndef MESH_LET_VIEWER_PASS_HLSL
#define MESH_LET_VIEWER_PASS_HLSL

#include "../Include/Meshlet.hlsli"
#include "../Include/CameraData.hlsli"
#include "../Include/TransformData.hlsli"

struct VertexInput
{
    float3 Position;
    float2 TexCoord;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD0;
};

ConstantBuffer<TransformData>   _TransformData          : register(b0);
ConstantBuffer<CameraData>      _CameraData             : register(b1);
StructuredBuffer<VertexInput>   _Vertices               : register(t0);
StructuredBuffer<Meshlet>       _Meshlets               : register(t1);
StructuredBuffer<uint>          _PackedPrimitiveIndices : register(t2);
StructuredBuffer<uint>          _UniqueVertexIndices    : register(t3);
Texture2D                       _MainTex                : register(t4);
SamplerState                    _MainTex_Sampler        : register(s0);

uint3 GetPrimitive(Meshlet m, uint index)
{
    return UnpackPrimitive(_PackedPrimitiveIndices[m.PrimOffset + index]);
}

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    localIndex = m.VertOffset + localIndex;
    return _UniqueVertexIndices.Load(localIndex * 4);
}

VertexOutput GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    VertexInput input = _Vertices[vertexIndex];
    VertexOutput output;
    float3 posWS = TransformLocalToWorld(_TransformData, input.Position);
    output.Position = mul(_CameraData.ViewProjection, float4(posWS, 1.0f));
    output.TexCoord = input.TexCoord;
    output.Color = float4(meshletIndex / 16.0f, meshletIndex / 16.0f, meshletIndex / 16.0f, 1);
    return output;
}

#endif