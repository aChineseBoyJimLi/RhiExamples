#ifndef RAY_TRACING_UTIL_HLSLI
#define RAY_TRACING_UTIL_HLSLI

float3 GetBarycentrics(float2 uv)
{
    return float3(1.0f - uv.x - uv.y, uv.x, uv.y);
}

float2 GetAttribute(float3 barycentrics, float2 v0, float2 v1, float2 v2)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

float3 GetAttribute(float3 barycentrics, float3 v0, float3 v1, float3 v2)
{
    return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

#endif