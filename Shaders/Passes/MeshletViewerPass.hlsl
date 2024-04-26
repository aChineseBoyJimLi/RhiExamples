#ifndef MESH_LET_VIEWER_PASS_HLSL
#define MESH_LET_VIEWER_PASS_HLSL

#include "../Include/Meshlet.hlsli"
#include "../Include/CameraData.hlsli"
#include "../Include/TransformData.hlsli"

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD0;
};

ConstantBuffer<TransformData>   _TransformData          : register(b0);
ConstantBuffer<CameraData>      _CameraData             : register(b1);
StructuredBuffer<float3>        _Vertices               : register(t0);
StructuredBuffer<float3>        _TexCoords              : register(t1);
StructuredBuffer<Meshlet>       _Meshlets               : register(t2);
StructuredBuffer<uint>          _PackedPrimitiveIndices : register(t3);
StructuredBuffer<uint>          _UniqueVertexIndices    : register(t4);
Texture2D                       _MainTex                : register(t5);
SamplerState                    _MainTex_Sampler        : register(s0);

uint3 GetPrimitive(Meshlet m, uint localIndex)
{
    return UnpackPrimitive(_PackedPrimitiveIndices[m.PrimOffset + localIndex]);
}

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    return _UniqueVertexIndices[m.VertOffset + localIndex];
}

VertexOutput GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    VertexOutput output;
    float3 posWS = TransformLocalToWorld(_TransformData, _Vertices[vertexIndex]);
    output.Position = TransformWorldToClip(_CameraData, posWS);
    output.TexCoord = _TexCoords[vertexIndex].xy;
    output.Color = float4(meshletIndex / 16.0f, meshletIndex / 16.0f, meshletIndex / 16.0f, 1);
    return output;
}

#endif