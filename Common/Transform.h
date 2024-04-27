#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vector>

struct TransformData
{
    glm::mat4 LocalToWorld;
    glm::mat4 WorldToLocal;

    static uint64_t GetAlignedByteSizes()
    {
        return (sizeof(TransformData) + 255) & ~255;
    }
};

// Translation, rotation, scaling transforms between different spaces
class Transform
{
public:
    Transform();
    ~Transform();

    void Reset();
    void SetParent(Transform* inParent);
    
    void SetLocalPosition(const glm::vec3& inPosition) { m_LocalPosition = inPosition; }
    void SetLocalRotation(const glm::quat& inRotation) { m_LocalRotation = glm::normalize(inRotation); }
    void SetLocalRotation(const glm::vec3& inEulerAngles) { m_LocalRotation = glm::quat(inEulerAngles); }
    void SetLocalRotation(const glm::vec3& inAxis, float inAngleDegrees) { m_LocalRotation = glm::angleAxis(glm::radians(inAngleDegrees), glm::normalize(inAxis)); }
    void SetLocalScale(const glm::vec3& inScale) { m_LocalScale = inScale;}
    
    void SetWorldPosition(const glm::vec3& inPosition);
    void SetWorldRotation(const glm::quat& inRotation);

    void Pitch(float inPatch) { m_LocalRotation = glm::rotate(m_LocalRotation, inPatch, glm::vec3(1, 0, 0)); }
    void Yaw(float inYaw) { m_LocalRotation = glm::rotate(m_LocalRotation, inYaw, glm::vec3(0, 1, 0)); }
    void Roll(float inRoll) { m_LocalRotation = glm::rotate(m_LocalRotation, inRoll, glm::vec3(0, 0, 1)); }
    void Translate(glm::vec3 inWorldPos) { m_LocalPosition += WorldToLocalPoint(inWorldPos);}
    void LookAt(const glm::vec3& inTarget); // the target's position in world space, the lookAt direction is (0,0,-1) in local space
    
    const glm::vec3& GetLocalPosition() const { return m_LocalPosition; }
    const glm::quat& GetLocalRotation() const { return m_LocalRotation; }
    const glm::vec3& GetLocalScale() const { return m_LocalScale; }
    glm::vec3 GetWorldPosition() const;
    glm::quat GetWorldRotation() const;

    glm::vec3 LocalToParentPoint(const glm::vec3& inVec) const;
    glm::vec3 ParentToLocalPoint(const glm::vec3& inVec) const;
    glm::vec3 LocalToWorldPoint(const glm::vec3& inVec) const;
    glm::vec3 WorldToLocalPoint(const glm::vec3& inVec) const;
    
    glm::vec3 LocalToParentVector(const glm::vec3& inVec) const; /*Includes Scale*/
    glm::vec3 ParentToLocalVector(const glm::vec3& inVec) const;
    glm::vec3 LocalToWorldVector(const glm::vec3& inVec) const;
    glm::vec3 WorldToLocalVector(const glm::vec3& inVec) const;

    glm::vec3 LocalToParentDirection(const glm::vec3& inVec) const; /*Ignores Scale*/
    glm::vec3 ParentToLocalDirection(const glm::vec3& inVec) const;
    glm::vec3 LocalToWorldDirection(const glm::vec3& inVec) const;
    glm::vec3 WorldToLocalDirection(const glm::vec3& inVec) const;

    glm::quat LocalToParentRotation(const glm::quat& inRotation) const; /*Ignores Scale*/
    glm::quat ParentToLocalRotation(const glm::quat& inRotation) const;
    glm::quat LocalToWorldRotation(const glm::quat& inRotation) const;
    glm::quat WorldToLocalRotation(const glm::quat& inRotation) const;

    glm::mat4 GetLocalToParentMatrix() const;
    glm::mat4 GetLocalToWorldMatrix() const;
    glm::mat4 GetParentToLocalMatrix() const;
    glm::mat4 GetWorldToLocalMatrix() const;

    void GetLocalToWorld3x4(float transform[3][4]) const;
    
    void GetLocalToParentMatrix(glm::mat4& outMatrix) const;
    void GetLocalToWorldMatrix(glm::mat4& outMatrix) const;
    void GetParentToLocalMatrix(glm::mat4& outMatrix) const;
    void GetWorldToLocalMatrix(glm::mat4& outMatrix) const;
    void GetTransformData(TransformData& outData) const;
    
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

    void SetLocalForward(const glm::vec3& inVec);
    void SetLocalRight(const glm::vec3& inVec);
    void SetLocalUp(const glm::vec3& inVec);

    glm::vec3 GetWorldForward() const; // forward direction (z-axis) in world space
    glm::vec3 GetWorldRight() const; // right direction (x-axis) in world space
    glm::vec3 GetWorldUp() const; // up direction (y-axis) in world space

    void SetWorldForward(const glm::vec3& inVec);
    void SetWorldRight(const glm::vec3& inVec);
    void SetWorldUp(const glm::vec3& inVec);

    // Rotate a vector by a quaternion
    static  glm::vec3 RotateVector(const glm::quat& inQuat, const glm::vec3& inVec);
    // Get a rotation matrix that rotates a vector called "from" into another vector called "to".
    static glm::mat3 GetRotationMatrixFromTo(const glm::vec3& from, const glm::vec3& to);
    // Get a rotation quaternion that rotates a vector called "from" into another vector called "to".
    static glm::quat GetRotationQuaternionFromTo(const glm::vec3& from, const glm::vec3& to);
    
private:
    Transform* m_Parent;
    std::vector<Transform*> m_Children;
    glm::vec3 m_LocalPosition; // position relative to parent
    glm::quat m_LocalRotation; // rotation relative to parent
    glm::vec3 m_LocalScale;    // scale relative to parent
};