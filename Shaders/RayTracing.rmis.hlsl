#include "Passes\RayTracingPass.hlsl"

[shader("miss")]
void RayMiss(inout Payload p)
{
    p.HitValue = float3(0.8f, 0.9f, 1.0f);
}

[shader("miss")]
void ShadowMiss(inout ShadowPayload p)
{
    p.Hit = false;
}