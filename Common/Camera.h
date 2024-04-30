#pragma once

#include <DirectXCollision.h>

#include "Transform.h"

struct CameraData
{
    glm::mat4 View;
    glm::mat4 Projection;
    glm::mat4 ViewProjection;
    glm::mat4 InvView;
    glm::mat4 InvProjection;
    glm::vec4 Position;

    static uint64_t GetAlignedByteSizes()
    {
        return (sizeof(CameraData) + 255) & ~255;
    }
};

// 8 corners of view frustum
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
    glm::vec3 Corners[8];
};

// 6 planes of view frustum
struct ViewFrustumPlanes
{
    // Plane equation: Ax + By + Cz + D = 0
    // A = normal.x B = normal.y C = normal.z D = dot(normal, pointOnPlane)
    // 0 Near, 1 Far, 2 Left, 3 Right, 4 Top, 5 Bottom
    glm::vec4 Planes[6];
};

class CameraBase
{
public:
    virtual ~CameraBase() = default;
    virtual glm::mat4 GetProjectionMatrix() const = 0; // view to clip space matrix
    virtual void GetViewFrustum(ViewFrustum& outFrustum) const = 0; // get view frustum in view space
    void GetViewFrustumPlanes(ViewFrustumPlanes& outFrustum) const; // get view frustum planes in view space
    void GetViewFrustumWorldSpace(ViewFrustum& outFrustum) const; // get view frustum in world space
    void GetViewFrustumPlanesWorldSpace(ViewFrustumPlanes& outFrustum) const; // get view frustum planes in world space
    static ViewFrustumPlanes Corners2Planes(const ViewFrustum& inFrustum); // get view frustum planes from corners
    static bool IsPointInFrustum(const ViewFrustumPlanes& inFrustum, const glm::vec3& inPoint); // check if point is in frustum
    static bool IsAABBInFrustum(const ViewFrustumPlanes& inFrustum, const glm::vec3& inMin, const glm::vec3& inMax); // check if AABB is in frustum
    static bool IsSphereInFrustum(const ViewFrustumPlanes& inFrustum, const glm::vec3& inCenter, float inRadius); // check if sphere is in frustum
    
    void GetCameraData(CameraData& outData) const;
    glm::mat4 GetViewMatrix() const ;  // world To view space matrix
    glm::mat4 GetViewProjectionMatrix() const; // world to clip space matrix
    
    Transform Transform;
    float AspectRatio {16.0f / 9.0f};
    float Near{0.1f};
    float Far{1000.f};
};

class CameraPerspective : public CameraBase
{
public:
    glm::mat4 GetProjectionMatrix() const override;
    void GetViewFrustum(ViewFrustum& outFrustum) const override;
    float Fov{45.f}; // in degree
};

class CameraOrthographic  : public CameraBase
{
public:
    glm::mat4 GetProjectionMatrix() const override;
    void GetViewFrustum(ViewFrustum& outFrustum) const override;
    float Size{1};
};