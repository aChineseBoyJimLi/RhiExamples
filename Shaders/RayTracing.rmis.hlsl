#include "Passes\RayTracingPass.hlsl"

[shader("miss")]
void RayMiss(inout Payload p : SV_RayPayload)
{
    p.HitValue = float3(0.8f, 0.9f, 1.0f);
}

[shader("miss")]
void ShadowMiss(inout ShadowPayload p : SV_RayPayload)
{
    p.Hit = false;
}