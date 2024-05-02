#include "Camera.h"

#include <DirectXCollision.h>

// Counter-clockwise order 3 points define a plane
// plane equation: Ax + By + Cz + D = 0
static glm::vec4 GetPlane(const glm::vec3 v0, const glm::vec3 v1, const glm::vec3 v2)
{
    glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    return glm::vec4(normal, -glm::dot(normal, v0));
}

ViewFrustumPlanes CameraBase::Corners2Planes(const ViewFrustum& inFrustum)
{
    ViewFrustumPlanes outFrustum;
    outFrustum.Planes[0] = GetPlane(inFrustum.Corners[2], inFrustum.Corners[1], inFrustum.Corners[0]); // Near
    outFrustum.Planes[1] = GetPlane(inFrustum.Corners[4], inFrustum.Corners[5], inFrustum.Corners[6]); // Far
    outFrustum.Planes[2] = GetPlane(inFrustum.Corners[7], inFrustum.Corners[3], inFrustum.Corners[0]); // Left
    outFrustum.Planes[3] = GetPlane(inFrustum.Corners[6], inFrustum.Corners[5], inFrustum.Corners[1]); // Right
    outFrustum.Planes[4] = GetPlane(inFrustum.Corners[7], inFrustum.Corners[6], inFrustum.Corners[2]); // Top
    outFrustum.Planes[5] = GetPlane(inFrustum.Corners[4], inFrustum.Corners[0], inFrustum.Corners[1]); // Bottom
    return outFrustum;
}

bool CameraBase::IsPointInFrustum(const ViewFrustumPlanes& inFrustum, const glm::vec3& inPoint) // check if point is in frustum
{
    for(int i = 0; i < 6; ++i)
    {
        float dis = glm::dot(inFrustum.Planes[i], glm::vec4(inPoint, 1.0f));
        if(dis > 0)
        {
            return false;
        }
    }
    return true;
}

bool CameraBase::IsAABBInFrustum(const ViewFrustumPlanes& inFrustum, const glm::vec3& inMin, const glm::vec3& inMax) // check if AABB is in frustum
{
    glm::vec3 corners[8];
    corners[0] = glm::vec3(inMin.x, inMin.y, inMin.z);
    corners[1] = glm::vec3(inMax.x, inMin.y, inMin.z);
    corners[2] = glm::vec3(inMin.x, inMax.y, inMin.z);
    corners[3] = glm::vec3(inMax.x, inMax.y, inMin.z);
    corners[4] = glm::vec3(inMin.x, inMin.y, inMax.z);
    corners[5] = glm::vec3(inMax.x, inMin.y, inMax.z);
    corners[6] = glm::vec3(inMin.x, inMax.y, inMax.z);
    corners[7] = glm::vec3(inMax.x, inMax.y, inMax.z);
    
    // if all the 8 points in the same outside a plane, then the AABB is outside the frustum
    for (int i = 0; i < 6; i++)
    {
        int outCount = 8;
        for (int j = 0; j < 8; j++)
        {
            float dis = glm::dot(inFrustum.Planes[i], glm::vec4(corners[j], 1.0f));
            if (dis > 0)
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

bool CameraBase::IsSphereInFrustum(const ViewFrustumPlanes& inFrustum, const glm::vec3& inCenter, float inRadius) // check if sphere is in frustum
{
    for (int i = 0; i < 6; i++)
    {
        float dis = glm::dot(inFrustum.Planes[i], glm::vec4(inCenter, 1.0f));
        if (dis > inRadius)
        {
            return false;
        }
    }
    return true;
}
    

glm::mat4 CameraBase::GetViewMatrix() const
{
    return Transform.GetWorldToLocalMatrix();
}

glm::mat4 CameraBase::GetViewProjectionMatrix() const
{
    return GetProjectionMatrix() * GetViewMatrix();
}
 
void CameraBase::GetCameraData(CameraData& outData) const 
{
    outData.View = GetViewMatrix();
    outData.Projection = GetProjectionMatrix();
    outData.ViewProjection = outData.Projection * outData.View;
    outData.InvView = glm::inverse(outData.View);
    outData.InvProjection = glm::inverse(outData.Projection);
    outData.Position = glm::vec4(Transform.GetWorldPosition(), 1.0f);
}

void CameraBase::GetViewFrustumPlanes(ViewFrustumPlanes& outFrustum) const
{
    ViewFrustum viewFrustum;
    GetViewFrustum(viewFrustum);
    outFrustum = Corners2Planes(viewFrustum);
}

void CameraBase::GetViewFrustumWorldSpace(ViewFrustum& outFrustum) const
{
    GetViewFrustum(outFrustum);
    for(uint32_t i = 0; i < 8 ; i++)
    {
        outFrustum.Corners[i] = Transform.LocalToWorldPoint(outFrustum.Corners[i]);
    }
}

void CameraBase::GetViewFrustumPlanesWorldSpace(ViewFrustumPlanes& outFrustum) const
{
    ViewFrustum viewFrustum;
    GetViewFrustumWorldSpace(viewFrustum);
    outFrustum = Corners2Planes(viewFrustum);
}

glm::mat4 CameraPerspective::GetProjectionMatrix() const
{
    return glm::perspectiveLH_ZO(glm::radians(Fov), AspectRatio, Near, Far);
}

void CameraPerspective::GetViewFrustum(ViewFrustum& outFrustum) const
{
    float nearPlaneHalfHeight = glm::tan(glm::radians(Fov) / 2) * Near;
    float nearPlaneHalfWidth = nearPlaneHalfHeight * AspectRatio;
    float farPlaneHalfHeight = glm::tan(glm::radians(Fov) / 2) * Far;
    float farPlaneHalfWidth = farPlaneHalfHeight * AspectRatio;
    
    outFrustum.Corners[0] = glm::vec3(-nearPlaneHalfWidth, -nearPlaneHalfHeight, Near); // near bottom left
    outFrustum.Corners[1] = glm::vec3(nearPlaneHalfWidth, -nearPlaneHalfHeight, Near); // near bottom right
    outFrustum.Corners[2] = glm::vec3(nearPlaneHalfWidth, nearPlaneHalfHeight, Near); // near top right
    outFrustum.Corners[3] = glm::vec3(-nearPlaneHalfWidth, nearPlaneHalfHeight, Near); // near top left
    outFrustum.Corners[4] = glm::vec3(-farPlaneHalfWidth, -farPlaneHalfHeight, Far); // far bottom left
    outFrustum.Corners[5] = glm::vec3(farPlaneHalfWidth, -farPlaneHalfHeight, Far); // far bottom right
    outFrustum.Corners[6] = glm::vec3(farPlaneHalfWidth, farPlaneHalfHeight, Far); // far top right
    outFrustum.Corners[7] = glm::vec3(-farPlaneHalfWidth, farPlaneHalfHeight, Far); // far top left
}

glm::mat4 CameraOrthographic::GetProjectionMatrix() const
{
    float width = Size * AspectRatio;
    float height = Size;
    return glm::orthoRH_ZO(-width, width, -height, height, Near, Far);
}

void CameraOrthographic::GetViewFrustum(ViewFrustum& outFrustum) const
{
    float farPlaneHalfHeight = Size / 2;
    float farPlaneHalfWidth = farPlaneHalfHeight * AspectRatio;

    outFrustum.Corners[0] = glm::vec3(-farPlaneHalfWidth, -farPlaneHalfHeight, Near); // near bottom left
    outFrustum.Corners[1] = glm::vec3(farPlaneHalfWidth, -farPlaneHalfHeight, Near); // near bottom right
    outFrustum.Corners[2] = glm::vec3(farPlaneHalfWidth, farPlaneHalfHeight, Near); // near top right
    outFrustum.Corners[3] = glm::vec3(-farPlaneHalfWidth, farPlaneHalfHeight, Near); // near top left
    outFrustum.Corners[4] = glm::vec3(-farPlaneHalfWidth, -farPlaneHalfHeight, Far); // far bottom left
    outFrustum.Corners[5] = glm::vec3(farPlaneHalfWidth, -farPlaneHalfHeight, Far); // far bottom right
    outFrustum.Corners[6] = glm::vec3(farPlaneHalfWidth, farPlaneHalfHeight, Far); // far top right
    outFrustum.Corners[7] = glm::vec3(-farPlaneHalfWidth, farPlaneHalfHeight, Far); // far top left
}
