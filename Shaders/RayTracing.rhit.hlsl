#include "Passes\RayTracingPass.hlsl"

[shader("closesthit")]
void ClosestHitTriangle(inout Payload p : SV_RayPayloadin, TrianglePrimitiveAttributes attribs : SV_IntersectionAttributes)
{
    uint instanceIndex = InstanceIndex();
    p.HitValue = float3(1.0, 1.0, 0.2);
}

// [shader("closesthit")]
// void ClosestHitAABB(inout Payload p : SV_RayPayloadin, ProceduralPrimitiveAttributes attribs)
// {
//     p.HitValue = float3(0.0, 0.0, 0.2);
// }
//
// [shader("intersection")]
// void ProceduralGeoPrim()
// {
//     
// }