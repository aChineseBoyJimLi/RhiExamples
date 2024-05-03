#include "Passes/MeshPipelinePass.hlsl"

groupshared Payload s_Payload;

[numthreads(AS_GROUP_SIZE, 1, 1)]
void main(uint groupId : SV_GroupID, uint groupThreadID : SV_GroupThreadID, uint dispatchThreadID : SV_DispatchThreadID)
{
    bool visible = false;

    // Check bounds of meshlet cull data resource
    if(dispatchThreadID < _MeshInfo.MeshletCount * _MeshInfo.InstanceCount)
    {
        uint instanceIndex = dispatchThreadID / _MeshInfo.MeshletCount;
        uint meshletIndex = dispatchThreadID % _MeshInfo.MeshletCount;
        visible = InstancedMeshletIsVisible(instanceIndex, meshletIndex, _ViewFrustum);  
    }

    // Compact visible meshlets into the export payload array
    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        s_Payload.InstancedMeshletIndices[index] = dispatchThreadID;
    }
    
    // Dispatch the required number of MS threadgroups to render the visible meshlets
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, s_Payload);
}