#include "Transform.h"

glm::vec3 RotateVector(const glm::quat& inQuat, const glm::vec3& inVec)
{
    glm::quat q = inQuat;
    glm::quat p = glm::quat(0, inVec.x, inVec.y, inVec.z);
    glm::quat result = q * p * glm::conjugate(q);
    return {result.x, result.y, result.z};
}

Transform::Transform()
    : m_Parent(nullptr)
    , m_LocalPosition(0, 0, 0)
    , m_LocalRotation(1, 0, 0, 0)
    , m_LocalScale(1, 1, 1)
{
    
}

void Transform::SetParent(Transform* inParent)
{
    if(m_Parent != nullptr)
    {
        if(m_Parent == inParent)
            return;
        m_Parent->m_Children.erase(std::remove(m_Parent->m_Children.begin(), m_Parent->m_Children.end(), this));
    }
    m_Parent = inParent;
    m_Parent->m_Children.push_back(this);
}

void Transform::SetLocalPosition(const glm::vec3& inPosition)
{
    m_LocalPosition = inPosition;
}

void Transform::SetLocalRotation(const glm::quat& inRotation)
{
    m_LocalRotation = inRotation;
}

void Transform::SetLocalRotation(const glm::vec3& inEulerAngles)
{
    m_LocalRotation = glm::quat(inEulerAngles);
}

void Transform::SetLocalRotation(const glm::vec3& inAxis, float inAngleDegrees)
{
    m_LocalRotation = glm::angleAxis(glm::radians(inAngleDegrees), inAxis);
}

void Transform::SetLocalScale(const glm::vec3& inScale)
{
    m_LocalScale = inScale;
}

glm::vec3 Transform::TransformLocalToParent(const glm::vec3& inVec) const
{
    return m_LocalPosition + RotateVector(m_LocalRotation, inVec * m_LocalScale);
}

glm::vec3 Transform::TransformParentToLocal(const glm::vec3& inVec) const
{
    return RotateVector(glm::conjugate(m_LocalRotation), inVec - m_LocalPosition) / m_LocalScale;
}

glm::vec3 Transform::TransformLocalToWorld(const glm::vec3& inVec) const
{
    glm::vec3 vec = TransformLocalToParent(inVec);
    const Transform* parent = m_Parent;
    while(parent != nullptr)
    {
        vec = parent->TransformLocalToParent(vec);
        parent = parent->m_Parent;
    }
    return vec;
}

glm::vec3 Transform::TransformWorldToLocal(const glm::vec3& inVec) const
{
    if(m_Parent == nullptr)
        return TransformParentToLocal(inVec);
    return TransformParentToLocal(m_Parent->TransformWorldToLocal(inVec));
}

void Transform::Reset()
{
    m_LocalPosition = glm::vec3(0, 0, 0);
    m_LocalRotation = glm::quat(1, 0, 0, 0);
    m_LocalScale = glm::vec3(1, 1, 1);
}

void Transform::GetTransformData(TransformData& outData) const
{
    GetLocalToWorldMatrix(outData.LocalToWorld);
    outData.WorldToLocal = glm::inverse(outData.LocalToWorld);
}

glm::mat4 Transform::GetLocalToParentMatrix() const
{
    // T * R * S
    return glm::translate(glm::mat4(1.0f), m_LocalPosition)
        * glm::mat4_cast(m_LocalRotation)
        * glm::scale(glm::mat4(1.0f), m_LocalScale);
}

glm::mat4 Transform::GetParentToLocalMatrix() const
{
    return glm::inverse(GetLocalToParentMatrix());
}

void Transform::GetLocalToParentMatrix(glm::mat4& outMatrix) const
{
    outMatrix = glm::translate(glm::mat4(1.0f), m_LocalPosition)
        * glm::mat4_cast(m_LocalRotation)
        * glm::scale(glm::mat4(1.0f), m_LocalScale);
}

void Transform::GetParentToLocalMatrix(glm::mat4& outMatrix) const
{
    outMatrix = glm::inverse(GetLocalToParentMatrix());
}

glm::mat4 Transform::GetLocalToWorldMatrix() const
{
    glm::mat4 localToWorld = GetLocalToParentMatrix();
    Transform* parent = m_Parent;
    while(parent != nullptr)
    {
        localToWorld = parent->GetLocalToParentMatrix() * localToWorld;
        parent = parent->m_Parent;
    }
    return localToWorld;
}

void Transform::GetLocalToWorldMatrix(glm::mat4& outMatrix) const
{
    GetLocalToParentMatrix(outMatrix);
    Transform* parent = m_Parent;
    while(parent != nullptr)
    {
        outMatrix = parent->GetLocalToParentMatrix() * outMatrix;
        parent = parent->m_Parent;
    }
}

glm::mat4 Transform::GetWorldToLocalMatrix() const
{
    return glm::inverse(GetLocalToWorldMatrix());
}

void Transform::GetWorldToLocalMatrix(glm::mat4& outMatrix) const
{
    outMatrix = glm::inverse(GetLocalToWorldMatrix());
}

glm::vec3 Transform::GetLocalForward() const
{
    return RotateVector(m_LocalRotation, glm::vec3(0, 0, 1));
}

glm::vec3 Transform::GetLocalRight() const
{
    return RotateVector(m_LocalRotation, glm::vec3(1, 0, 0));
}

glm::vec3 Transform::GetLocalUp() const
{
    return RotateVector(m_LocalRotation, glm::vec3(0, 1, 0));
}

glm::vec3 Transform::GetWorldForward() const
{
    return TransformLocalToWorld(glm::vec3(0, 0, 1));
}

glm::vec3 Transform::GetWorldRight() const
{
    return TransformLocalToWorld(glm::vec3(1, 0, 0));
}

glm::vec3 Transform::GetWorldUp() const
{
    return TransformLocalToWorld(glm::vec3(0, 1, 0));
}