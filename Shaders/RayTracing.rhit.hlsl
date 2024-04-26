#include "Passes\RayTracingPass.hlsl"

[shader("closesthit")]
void ClosestHitTriangle(inout Payload p : SV_RayPayloadin, TrianglePrimitiveAttributes attribs : SV_IntersectionAttributes)
{
    uint3 indices = GetTriangleIndices(PrimitiveIndex());
    float3 barycentrics = GetBarycentrics(attribs.UV);

    float3 normal = GetNormal(indices, barycentrics);
    float3 hitPositionWS = HitWorldPosition();

    TransformData transformData = _MeshInstanceTransforms[InstanceIndex()];
    float3 normalWS = TransformLocalToWorldNormal(transformData, normal);

    float3 incident = normalize(-_LightData.LightDirection);
    p.HitValue = _LightData.LightIntensity * _LightData.LightColor * saturate(dot(normalWS, incident));
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