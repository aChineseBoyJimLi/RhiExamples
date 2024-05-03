#include "Passes/MeshPipelinePass.hlsl"

[NumThreads(MS_GROUP_SIZE, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload Payload payload,
    out indices uint3 tris[68],
    out vertices VertexOutput verts[MS_GROUP_SIZE]
)
{
    // Load the meshlet from the AS payload data
    uint instancedMeshletIndex = payload.InstancedMeshletIndices[gid];

    // Catch any out-of-range indices (in case too many MS threadgroups were dispatched from AS)
    if (instancedMeshletIndex >= _MeshInfo.MeshletCount * _MeshInfo.InstanceCount)
        return;
    
    uint instanceIndex = instancedMeshletIndex / _MeshInfo.MeshletCount;
    uint meshletIndex = instancedMeshletIndex % _MeshInfo.MeshletCount;

    InstanceData instanceData = _InstanceData[instanceIndex];
    Meshlet m = _Meshlets[meshletIndex];
    
    SetMeshOutputCounts(m.VertCount, m.PrimCount);
    
    if(gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid);
    }
    if(gtid < m.VertCount)
    {
        uint vertexIndex = GetVertexIndex(m, gtid);
        verts[gtid] = GetVertexAttributes(instanceData.Transform, meshletIndex, vertexIndex);
    }
}
