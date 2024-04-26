#ifndef CAMERA_DATA_HLSLI
#define CAMERA_DATA_HLSLI

struct CameraData
{
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4x4 InvView;
    float4x4 InvProjection;
    float3  Position;
};

float3 TransformWorldToView(in CameraData Data, in float3 posWS)
{
    return mul(float4(posWS, 1.0f), Data.View).xyz;
}

float4 TransformWorldToClip(in CameraData Data, in float3 posWS)
{
    return mul(float4(posWS, 1.0f), Data.ViewProjection);
}

float4 TransformViewToClip(in CameraData Data, in float3 posVS)
{
    return mul(float4(posVS, 1.0f), Data.Projection);
}

void GenerateCameraRay(out float3 origin, out float3 direction, in float2 screenUV, in CameraData cameraData)
{
    origin = cameraData.Position;
    float4 target = mul(float4(screenUV.x, screenUV.y, 1, 1), cameraData.InvProjection);
    direction = mul(target.xyz, (float3x3)cameraData.InvView);
}

#endif // CAMERA_DATA_HLSLI