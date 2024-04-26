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
    return mul(Data.View, float4(posWS, 1.0f)).xyz;
}

float4 TransformWorldToClip(in CameraData Data, in float3 posWS)
{
    return mul(Data.ViewProjection, float4(posWS, 1.0f));
}

float4 TransformViewToClip(in CameraData Data, in float3 posVS)
{
    return mul(Data.Projection, float4(posVS, 1.0f));
}

float4 TransformClipToView(in CameraData Data, float4 posCS)
{
    return mul(Data.InvProjection, posCS);
}

float3 TransformViewToWorld(in CameraData Data, in float3 posVS)
{
    return mul(Data.InvView, float4(posVS, 1.0f)).xyz;
}

float3 TransformWorldToViewDir(in CameraData Data, in float3 dir)
{
    return normalize(mul((float3x3)Data.View, dir));
}

float3 TransformViewToWorldDir(in CameraData Data, in float3 dir)
{
    return normalize(mul((float3x3)Data.InvView, dir));
}

void GenerateCameraRay(out float3 origin, out float3 direction, in float2 screenUV, in CameraData cameraData)
{
    float2 d = screenUV * 2.0 - 1.0;
    origin = cameraData.Position;
    float4 target = TransformClipToView(cameraData, float4(d.x, d.y, 1, 1));
    direction = TransformViewToWorldDir(cameraData, target.xyz);
}

#endif // CAMERA_DATA_HLSLI