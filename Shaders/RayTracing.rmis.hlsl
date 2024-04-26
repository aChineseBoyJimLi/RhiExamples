#include "Passes\RayTracingPass.hlsl"

[shader("miss")]
void RayMiss(inout Payload p : SV_RayPayload)
{
    p.HitValue = float3(0.0, 0.0, 0.2);
}