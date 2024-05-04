#include "Passes\RayTracingPass.hlsl"

[shader("raygeneration")]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchSize = DispatchRaysDimensions();

    const float2 pixelCenter = float2(launchIndex.xy) + float2(0.5, 0.5);
    const float2 inUV = pixelCenter/float2(launchSize.xy);

    float3 o, dir;
    GenerateCameraRay(o, dir, inUV, _CameraData);
    
    float4 color = TraceRadianceRay(o, dir, 0);
    _Output[launchIndex.xy] = color;
}