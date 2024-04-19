#ifndef CAMERA_DATA_HLSLI
#define CAMERA_DATA_HLSLI

struct CameraData
{
    float4x4 View;
    float4x4 Projection;
    float4x4 ViewProjection;
    float4x4 InvView;
    float4x4 InvProjection;
    float3 Position;
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

#endif // CAMERA_DATA_HLSLI