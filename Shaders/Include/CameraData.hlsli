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
    // 0 near bottom left
    // 1 near bottom right
    // 2 near top right
    // 3 near top left
    // 4 far bottom left
    // 5 far bottom right
    // 6 far top right
    // 7 far top left
    float4 Corners[8]; 
};

struct ViewFrustumPlane
{
    // 0 Near, 1 Far, 2 Left, 3 Right, 4 Top, 5 Bottom
    float4 Planes[6]; 
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

float4 GetPlane(in float3 v0, in float3 v1, in float3 v2)
{
    float3 normal = normalize(cross(v1 - v0, v2 - v0));
    return float4(normal, -dot(normal, v2));
}

ViewFrustumPlane GetViewFrustumPlanes(in ViewFrustum frustum)
{
    ViewFrustumPlane outFrustum;
    outFrustum.Planes[0] = GetPlane(frustum.Corners[2].xyz, frustum.Corners[1].xyz, frustum.Corners[0].xyz); // Near
    outFrustum.Planes[1] = GetPlane(frustum.Corners[4].xyz, frustum.Corners[5].xyz, frustum.Corners[6].xyz); // Far
    outFrustum.Planes[2] = GetPlane(frustum.Corners[7].xyz, frustum.Corners[3].xyz, frustum.Corners[0].xyz); // Left
    outFrustum.Planes[3] = GetPlane(frustum.Corners[6].xyz, frustum.Corners[5].xyz, frustum.Corners[1].xyz); // Right
    outFrustum.Planes[4] = GetPlane(frustum.Corners[7].xyz, frustum.Corners[6].xyz, frustum.Corners[2].xyz); // Top
    outFrustum.Planes[5] = GetPlane(frustum.Corners[4].xyz, frustum.Corners[0].xyz, frustum.Corners[1].xyz); // Bottom
    return outFrustum;
}

bool IsPointInFrustum(in ViewFrustumPlane frustum, in float3 p)
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

bool IsAABBInFrustum(in ViewFrustumPlane frustum, in float3 min, in float3 max)
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
    
    // if all the 8 points in the same outside of a plane, then the AABB is outside of the frustum
    for (int i = 0; i < 6; i++)
    {
        int outCount = 8;
        for (int j = 0; j < 8; j++)
        {
            if (dot(frustum.Planes[i], float4(corners[j], 1.0f)) > 0)
            {
                outCount--;
            }
        }
        if (outCount == 0)
        {
            return false;
        }
    }

    return true;
}

bool IsSphereInFrustum(in ViewFrustumPlane frustum, in float3 center, in float radius)
{
    for (int i = 0; i < 6; i++)
    {
        if (dot(frustum.Planes[i], float4(center, 1.0f)) > radius)
        {
            return false;
        }
    }
    return true;
}

#endif // CAMERA_DATA_HLSLI