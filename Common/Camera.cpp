#include "Camera.h"

#include <DirectXCollision.h>

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
    outData.View = glm::transpose(GetViewMatrix());
    outData.Projection = glm::transpose(GetProjectionMatrix());
    outData.ViewProjection = outData.View * outData.Projection;
    outData.InvView = glm::transpose(outData.View);
    outData.InvProjection = glm::transpose(outData.Projection);
    outData.Position = glm::vec4(Transform.GetWorldPosition(), 1.0f);
}

glm::mat4 CameraPerspective::GetProjectionMatrix() const
{
    return glm::perspectiveRH_ZO(glm::radians(Fov), AspectRatio, Near, Far);
}

glm::mat4 CameraOrthographic::GetProjectionMatrix() const
{
    float width = Size * AspectRatio;
    float height = Size;
    return glm::orthoRH_ZO(-width, width, -height, height, Near, Far);
}
