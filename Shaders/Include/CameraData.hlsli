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

struct ViewFrustum
{
    float4 Planes[6]; // Near, Far, Left, Right, Top, Bottom
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

bool IsPointInFrustum(in ViewFrustum frustum, in float3 p)
{
    for (int i = 0; i < 6; i++)
    {
        if (dot(frustum.Planes[i], float4(p, 1.0f)) > 0)
        {
            return false;
        }
    }
    return true;
}

bool IsAABBInFrustum(in ViewFrustum frustum, in float3 min, in float3 max)
{
    float3 corners[8];
    corners[0] = float3(min.x, min.y, min.z);
    corners[1] = float3(max.x, min.y, min.z);
    corners[2] = float3(min.x, max.y, min.z);
    corners[3] = float3(max.x, max.y, min.z);
    corners[4] = float3(min.x, min.y, max.z);
    corners[5] = float3(max.x, min.y, max.z);
    corners[6] = float3(min.x, max.y, max.z);
    corners[7] = float3(max.x, max.y, max.z);

    
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if(IsPointInFrustum(frustum, corners[j]))
            {
                return true;
            }
        }
    }
    return false;
}

#endif // CAMERA_DATA_HLSLI