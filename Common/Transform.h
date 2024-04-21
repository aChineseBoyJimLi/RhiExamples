#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vector>

struct TransformData
{
    glm::mat4 LocalToWorld;
    glm::mat4 WorldToLocal;
};

glm::vec3 RotateVector(const glm::quat& inQuat, const glm::vec3& inVec);

// Translation, rotation, scaling transforms between different spaces
class Transform
{
public:
    Transform();
    ~Transform() = default;
    
    void SetParent(Transform* inParent);
    
    void Reset();
    void SetLocalPosition(const glm::vec3& inPosition);
    void SetLocalRotation(const glm::quat& inRotation);
    void SetLocalRotation(const glm::vec3& inEulerAngles);
    void SetLocalRotation(const glm::vec3& inAxis, float inAngleDegrees);
    void SetLocalScale(const glm::vec3& inScale);
    
    const glm::vec3& GetLocalPosition() const { return m_LocalPosition; }
    const glm::quat& GetLocalRotation() const { return m_LocalRotation; }
    const glm::vec3& GetLocalScale() const { return m_LocalScale; }
    
    glm::mat4 GetLocalToParentMatrix() const;
    glm::mat4 GetLocalToWorldMatrix() const;
    glm::mat4 GetParentToLocalMatrix() const;
    glm::mat4 GetWorldToLocalMatrix() const;
    void GetLocalToParentMatrix(glm::mat4& outMatrix) const;
    void GetLocalToWorldMatrix(glm::mat4& outMatrix) const;
    void GetParentToLocalMatrix(glm::mat4& outMatrix) const;
    void GetWorldToLocalMatrix(glm::mat4& outMatrix) const;
    void GetTransformData(TransformData& outData) const;
    
    glm::vec3 TransformLocalToParent(const glm::vec3& inVec) const;
    glm::vec3 TransformParentToLocal(const glm::vec3& inVec) const;
    glm::vec3 TransformLocalToWorld(const glm::vec3& inVec) const;
    glm::vec3 TransformWorldToLocal(const glm::vec3& inVec) const;
    
    /* right-handed coordinate system
     * 
     *      up(y-axis)
     *      |
     *      |
     *      /-------> right(x-axis)
     *     /
     *    / forward(z-axis)
     */
    glm::vec3 GetLocalForward() const; // forward direction (z-axis) relative to parent
    glm::vec3 GetLocalRight() const; // right direction (x-axis) relative to parent
    glm::vec3 GetLocalUp() const; // up direction (y-axis) relative to parent

    glm::vec3 GetWorldForward() const; // forward direction (z-axis) in world space
    glm::vec3 GetWorldRight() const; // right direction (x-axis) in world space
    glm::vec3 GetWorldUp() const; // up direction (y-axis) in world space

private:
    Transform* m_Parent;
    std::vector<Transform*> m_Children;
    glm::vec3 m_LocalPosition; // position relative to parent
    glm::quat m_LocalRotation; // rotation relative to parent
    glm::vec3 m_LocalScale;    // scale relative to parent
};