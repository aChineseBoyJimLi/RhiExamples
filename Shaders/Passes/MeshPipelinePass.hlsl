#ifndef MESH_LET_VIEWER_PASS_HLSL
#define MESH_LET_VIEWER_PASS_HLSL

#include "../Include/Meshlet.hlsli"
#include "../Include/CameraData.hlsli"
#include "../Include/TransformData.hlsli"

struct Payload
{
    uint InstancedMeshletIndices[AS_GROUP_SIZE];
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD0;
};

struct InstanceData
{
    TransformData	Transform;
};

ConstantBuffer<CameraData>          _CameraData             : register(b0);
ConstantBuffer<ViewFrustum>         _ViewFrustum            : register(b1);
ConstantBuffer<MeshInfo>            _MeshInfo               : register(b2);

// Meshlet data
StructuredBuffer<float4>            _Vertices               : register(t0);
StructuredBuffer<float2>            _TexCoords              : register(t1);
StructuredBuffer<Meshlet>           _Meshlets               : register(t2);
StructuredBuffer<uint>              _PackedPrimitiveIndices : register(t3);
ByteAddressBuffer                   _UniqueVertexIndices    : register(t4);
StructuredBuffer<MeshletCullData>	_MeshletCullData	    : register(t5);

// Instance data
StructuredBuffer<InstanceData>		_InstanceData	        : register(t6);

bool InstancedMeshletIsVisible(uint instanceIndex, uint meshletIndex, ViewFrustum viewFrustumWS)
{
    InstanceData instanceData = _InstanceData[instanceIndex];
    MeshletCullData cullDataInstance = _MeshletCullData[meshletIndex];

    ViewFrustum viewFrustumLS;
    for(uint i = 0; i < 8; ++i)
    {
        viewFrustumLS.Corners[i].xyz = TransformWorldToLocal(instanceData.Transform, viewFrustumWS.Corners[i].xyz);
    }

    ViewFrustumPlane planes = GetViewFrustumPlanes(viewFrustumLS);
    return IsSphereInFrustum(planes, cullDataInstance.BoundingSphere.xyz, cullDataInstance.BoundingSphere.w);
}

uint3 GetPrimitive(Meshlet m, uint localIndex)
{
    return UnpackPrimitive(_PackedPrimitiveIndices[m.PrimOffset + localIndex]);
}

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    // mesh vertex index is 32-bit
    uint index = _UniqueVertexIndices.Load((m.VertOffset + localIndex) * 4);
    return index;
}

VertexOutput GetVertexAttributes(TransformData transform, uint meshletIndex, uint vertexIndex)
{
    VertexOutput output;
    float3 posWS = TransformLocalToWorld(transform, _Vertices[vertexIndex].xyz);
    output.Position = TransformWorldToClip(_CameraData, posWS);
    output.TexCoord = _TexCoords[vertexIndex].xy;
    output.Color = float4(meshletIndex / 16.0f, meshletIndex / 16.0f, meshletIndex / 16.0f, 1);
    return output;
}

#endif