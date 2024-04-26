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

class CameraBase
{
public:
    virtual ~CameraBase() = default;
    virtual glm::mat4 GetProjectionMatrix() const = 0; // view to clip space matrix
    
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
    float Fov{45.f}; // in degree
};

class CameraOrthographic  : public CameraBase
{
public:
    glm::mat4 GetProjectionMatrix() const override;
    float Size{1};
};