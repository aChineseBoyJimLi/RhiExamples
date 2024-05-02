#include "Include/CameraData.hlsli"
#include "Include/TransformData.hlsli"
#include "Include/Common.hlsli"

struct AABB
{
    float4 Min;
    float4 Max;
};

struct InstanceData
{
    TransformData	Transform;
    uint			MatIndex;
    uint			Padding0;
    uint			Padding1;
    uint			Padding2;
};

ConstantBuffer<CameraData>                          _CameraData     : register(b0);
ConstantBuffer<ViewFrustum>                         _ViewFrustum    : register(b1);
ConstantBuffer<AABB>                                _AABB           : register(b2);
StructuredBuffer<InstanceData>                      _InstancesData  : register(t0);
RWStructuredBuffer<IndexedIndirectDrawCommand>     _IndirectCommands : register(u0); //  All indirect draw commands

[numthreads(128, 1, 1)]
void main(uint groupId : SV_GroupID, uint groupThreadID : SV_GroupThreadID)
{
    uint index = groupId * 128 + groupThreadID;
    if (index >= 4096)
        return;
    
    InstanceData instanceData = _InstancesData[index];
    ViewFrustum viewFrustumLS;
    for(uint i = 0; i < 8; ++i)
    {
        viewFrustumLS.Corners[i].xyz = TransformWorldToLocal(instanceData.Transform, _ViewFrustum.Corners[i].xyz);
    }
    ViewFrustumPlane planes = GetViewFrustumPlanes(viewFrustumLS);
    if (!IsAABBInFrustum(planes, _AABB.Min.xyz, _AABB.Max.xyz))
    {
        _IndirectCommands[index].InstanceCount = 0;
    }
}