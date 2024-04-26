#include "Passes\RayTracingPass.hlsl"

[shader("raygeneration")]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchSize = DispatchRaysDimensions();

    const float2 pixelCenter = float2(launchIndex.xy) + float2(0.5, 0.5);
    const float2 inUV = pixelCenter/float2(launchSize.xy);
    
    RayDesc rayDesc;
    GenerateCameraRay(rayDesc.Origin, rayDesc.Direction, inUV, _CameraData);
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    Payload payload;
    TraceRay(_AccelStructure, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

    _Output[launchIndex.xy] = float4(payload.HitValue, 1);
}