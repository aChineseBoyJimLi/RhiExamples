#include "Include/CameraData.hlsli"
#include "Include/TransformData.hlsli"

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

struct IndirectCommand
{
    uint2 CbvAddress;
    // DrawIndexedCommand DrawIndexedArguments;
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
    uint Padding;
};

ConstantBuffer<CameraData>              _CameraData     : register(b0);
ConstantBuffer<ViewFrustum>             _ViewFrustum    : register(b1);
StructuredBuffer<InstanceData>          _InstancesData  : register(t0);
StructuredBuffer<IndirectCommand>       _InputCommands  : register(t1);    //  All indirect draw commands
AppendStructuredBuffer<IndirectCommand> _OutputCommands : register(u0);      // Remaining indirect commands and its count

[numthreads(128, 1, 1)]
void main(uint groupId : SV_GroupID, uint groupThreadID : SV_GroupThreadID)
{
    uint index = groupId * 128 + groupThreadID;
    if (index >= 4096)
        return;

    IndirectCommand cmd = _InputCommands[index];
    InstanceData instanceData = _InstancesData[index];
    ViewFrustum viewFrustumLS;
    for(uint i = 0; i < 8; ++i)
    {
        viewFrustumLS.Corners[i].xyz = TransformWorldToLocal(instanceData.Transform, _ViewFrustum.Corners[i].xyz);
    }
    ViewFrustumPlane planes = GetViewFrustumPlanes(viewFrustumLS);
    if (IsAABBInFrustum(planes, instanceData.Min, instanceData.Max))
    {
        _OutputCommands.Append(cmd);
    }
}