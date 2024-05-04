#include "Passes\RayTracingPass.hlsl"

[shader("closesthit")]
void ClosestHitTriangle(inout Payload payload, in TrianglePrimitiveAttributes attribs)
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
void ClosestHitProceduralPrim(inout Payload payload, in ProceduralPrimitiveAttributes attr)
{
    float3 planeNormalWS = float3(0, 1, 0);
    float3 hitPositionWS = HitWorldPosition();
    MaterialData material = _MaterialData[_InstanceData[InstanceIndex()].MatIndex];
    payload.HitValue = SufaceLighting(hitPositionWS, planeNormalWS, material,  payload.Depth);
}

[shader("intersection")]
void ProceduralPlanePrim()
{
    float3 planeNormalWS = float3(0, 1, 0);
    float3 planePointWS = float3(0, 0, 0);
    
    float3 rayDir = WorldRayDirection();
    float3 rayOrigin = WorldRayOrigin();
    if(abs(dot(rayDir, planeNormalWS)) > EPSILON)
    {
        float thit = dot(planeNormalWS, planePointWS - rayOrigin) / dot(planeNormalWS, rayDir);
        ProceduralPrimitiveAttributes attr;
        attr.WorldNormal = planeNormalWS;
        if(thit > RayTMin())
        {
            ReportHit(thit, 0, attr);
        }
    }
}