#include "Passes\RayTracingPass.hlsl"

[shader("closesthit")]
void ClosestHitTriangle(inout Payload payload : SV_RayPayloadin, TrianglePrimitiveAttributes attribs : SV_IntersectionAttributes)
{
    uint3 indices = GetTriangleIndices(PrimitiveIndex());
    float3 barycentrics = GetBarycentrics(attribs.UV);

    float3 normal = GetNormal(indices, barycentrics);
    float3 hitPositionWS = HitWorldPosition();

    TransformData transformData = _InstanceData[InstanceIndex()].Transform;
    MaterialData material = _MaterialData[_InstanceData[InstanceIndex()].MatIndex];
    
    float3 normalWS = TransformLocalToWorldNormal(transformData, normal);
    payload.HitValue = SufaceLighting(hitPositionWS, normalWS, material,  payload.Depth);
}

[shader("closesthit")]
void ClosestHitProceduralPrim(inout Payload payload : SV_RayPayloadin, in ProceduralPrimitiveAttributes attr)
{
    float3 hitPositionWS = HitWorldPosition();

    TransformData transformData = _InstanceData[InstanceIndex()].Transform;
    MaterialData material = _MaterialData[_InstanceData[InstanceIndex()].MatIndex];
    
    float3 normalWS = TransformLocalToWorldNormal(transformData, attr.Normal);
    payload.HitValue = SufaceLighting(hitPositionWS, normalWS, material,  payload.Depth);
}

[shader("intersection")]
void ProceduralPlanePrim()
{
    float thit = 0;
    ProceduralPrimitiveAttributes attr;
    ReportHit(thit, 0, attr);
}