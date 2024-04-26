#include "Passes/MeshletViewerPass.hlsl"

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint3 tris[68],
    out vertices VertexOutput verts[128]
)
{
    Meshlet m = _Meshlets[gid];
    SetMeshOutputCounts(m.VertCount, m.PrimCount);
    if(gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid);
    }
    if(gtid < m.VertCount)
    {
        uint vertexIndex = GetVertexIndex(m, gtid);
        verts[gtid] = GetVertexAttributes(gid, vertexIndex);
    }
}
