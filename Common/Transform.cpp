#include "Transform.h"

Transform::Transform()
    : m_Parent(nullptr)
    , m_LocalPosition(0, 0, 0)
    , m_LocalRotation(1, 0, 0, 0)
    , m_LocalScale(1, 1, 1)
{
    
}

Transform::~Transform()
{
    for(auto child : m_Children)
    {
        child->m_Parent = m_Parent;
        if(m_Parent != nullptr)
            m_Parent->m_Children.push_back(child);
    }
    if(m_Parent != nullptr)
        m_Parent->m_Children.erase(std::remove(m_Parent->m_Children.begin(), m_Parent->m_Children.end(), this));
}

void Transform::SetParent(Transform* inParent)
{
    if(m_Parent != nullptr)
    {
        if(m_Parent == inParent || inParent == this)
            return;
        m_Parent->m_Children.erase(std::remove(m_Parent->m_Children.begin(), m_Parent->m_Children.end(), this));
    }
    m_Parent = inParent;
    m_Parent->m_Children.push_back(this);
}

void Transform::LookAt(const glm::vec3& inTarget)
{
    const glm::vec3 localPosition = WorldToLocalPoint(inTarget);
    const glm::vec3 localForward = glm::normalize(localPosition);
    SetLocalForward(localForward);
}

void Transform::SetWorldPosition(const glm::vec3& inPosition)
{
    if(m_Parent == nullptr)
    {
        SetLocalPosition(inPosition);
        return;
    }
        
    SetLocalPosition(m_Parent->WorldToLocalPoint(inPosition));
}

void Transform::SetWorldRotation(const glm::quat& inRotation)
{
    if(m_Parent == nullptr)
    {
        SetLocalRotation(inRotation);
        return;
    }
    SetLocalRotation(m_Parent->WorldToLocalRotation(inRotation));
}

glm::vec3 Transform::GetWorldPosition() const
{
    if(m_Parent == nullptr)
        return m_LocalPosition;
    return m_Parent->LocalToWorldPoint(m_LocalPosition);
}

glm::quat Transform::GetWorldRotation() const
{
    if(m_Parent == nullptr)
        return m_LocalRotation;
    return m_Parent->LocalToWorldRotation(m_LocalRotation);
}

glm::vec3 Transform::LocalToParentPoint(const glm::vec3& inVec) const
{
    return m_LocalPosition + RotateVector(m_LocalRotation, inVec * m_LocalScale);
}

glm::vec3 Transform::ParentToLocalPoint(const glm::vec3& inVec) const
{
    return RotateVector(glm::conjugate(m_LocalRotation), inVec - m_LocalPosition) / m_LocalScale;
}

glm::vec3 Transform::LocalToWorldPoint(const glm::vec3& inVec) const
{
    glm::vec3 vec = LocalToParentPoint(inVec);
    const Transform* parent = m_Parent;
    while(parent != nullptr)
    {
        vec = parent->LocalToParentPoint(vec);
        parent = parent->m_Parent;
    }
    return vec;
}

glm::vec3 Transform::WorldToLocalPoint(const glm::vec3& inVec) const
{
    if(m_Parent == nullptr)
        return ParentToLocalPoint(inVec);
    return ParentToLocalPoint(m_Parent->WorldToLocalPoint(inVec));
}

glm::vec3 Transform::LocalToParentVector(const glm::vec3& inVec) const
{
    return RotateVector(m_LocalRotation, inVec * m_LocalScale);
}

glm::vec3 Transform::ParentToLocalVector(const glm::vec3& inVec) const
{
    return RotateVector(glm::conjugate(m_LocalRotation), inVec) / m_LocalScale;
}

glm::vec3 Transform::LocalToWorldVector(const glm::vec3& inVec) const
{
    glm::vec3 vec = LocalToParentVector(inVec);
    const Transform* parent = m_Parent;
    while(parent != nullptr)
    {
        vec = parent->LocalToParentVector(vec);
        parent = parent->m_Parent;
    }
    return vec;
}

glm::vec3 Transform::WorldToLocalVector(const glm::vec3& inVec) const
{
    if(m_Parent == nullptr)
        return ParentToLocalVector(inVec);
    return ParentToLocalVector(m_Parent->WorldToLocalVector(inVec));
}

glm::vec3 Transform::LocalToParentDirection(const glm::vec3& inVec) const
{
    return RotateVector(m_LocalRotation, glm::normalize(inVec));
}

glm::vec3 Transform::ParentToLocalDirection(const glm::vec3& inVec) const
{
    return RotateVector(glm::conjugate(m_LocalRotation), glm::normalize(inVec));
}

glm::vec3 Transform::LocalToWorldDirection(const glm::vec3& inVec) const
{
    glm::vec3 vec = LocalToParentDirection(inVec);
    const Transform* parent = m_Parent;
    while(parent != nullptr)
    {
        vec = parent->LocalToParentDirection(vec);
        parent = parent->m_Parent;
    }
    return vec;
}

glm::vec3 Transform::WorldToLocalDirection(const glm::vec3& inVec) const
{
    if(m_Parent == nullptr)
        return ParentToLocalDirection(inVec);
    return ParentToLocalDirection(m_Parent->WorldToLocalDirection(inVec));
}

glm::quat Transform::LocalToParentRotation(const glm::quat& inRotation) const
{
    return glm::normalize(m_LocalRotation * glm::normalize(inRotation));
}

glm::quat Transform::ParentToLocalRotation(const glm::quat& inRotation) const
{
    return glm::normalize(glm::conjugate(m_LocalRotation) * glm::normalize(inRotation));
}

glm::quat Transform::LocalToWorldRotation(const glm::quat& inRotation) const
{
    glm::quat q = LocalToParentRotation(inRotation);
    const Transform* parent = m_Parent;
    while(parent != nullptr)
    {
        q = parent->LocalToParentRotation(q);
        parent = parent->m_Parent;
    }
    return q;
}

glm::quat Transform::WorldToLocalRotation(const glm::quat& inRotation) const
{
    if(m_Parent == nullptr)
        return ParentToLocalRotation(inRotation);
    return ParentToLocalRotation(m_Parent->WorldToLocalRotation(inRotation));
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
    return glm::scale(glm::mat4(1.0f), 1.0f / m_LocalScale)
        * glm::mat4_cast(glm::conjugate(m_LocalRotation))
        * glm::translate(glm::mat4(1.0f), -m_LocalPosition);
}

void Transform::GetLocalToParentMatrix(glm::mat4& outMatrix) const
{
    outMatrix = glm::translate(glm::mat4(1.0f), m_LocalPosition)
        * glm::mat4_cast(m_LocalRotation)
        * glm::scale(glm::mat4(1.0f), m_LocalScale);
}

void Transform::GetParentToLocalMatrix(glm::mat4& outMatrix) const
{
    outMatrix = GetParentToLocalMatrix();
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
    if(m_Parent == nullptr)
        return GetParentToLocalMatrix();
    return GetParentToLocalMatrix() * m_Parent->GetWorldToLocalMatrix();
}

void Transform::GetWorldToLocalMatrix(glm::mat4& outMatrix) const
{
    outMatrix = GetWorldToLocalMatrix();
}

void Transform::GetLocalToWorld3x4(float transform[3][4]) const
{
    glm::mat4 matrix = GetLocalToWorldMatrix(); 
    transform[0][0] = matrix[0][0]; transform[0][1] = matrix[0][1]; transform[0][2] = matrix[0][2];     transform[0][3] = matrix[3][0]; 
    transform[1][0] = matrix[1][0]; transform[1][1] = matrix[1][1]; transform[1][2] = matrix[1][2];     transform[1][3] = matrix[3][1]; 
    transform[2][0] = matrix[2][0]; transform[2][1] = matrix[2][1]; transform[2][2] = matrix[2][2];     transform[2][3] = matrix[3][2]; 
}

glm::vec3 Transform::GetLocalForward() const
{
    return glm::normalize(RotateVector(m_LocalRotation, glm::vec3(0, 0, 1)));
}

glm::vec3 Transform::GetLocalRight() const
{
    return glm::normalize(RotateVector(m_LocalRotation, glm::vec3(1, 0, 0)));
}

glm::vec3 Transform::GetLocalUp() const
{
    return glm::normalize(RotateVector(m_LocalRotation, glm::vec3(0, 1, 0)));
}

glm::vec3 Transform::GetWorldForward() const
{
    return glm::normalize(LocalToWorldVector(glm::vec3(0, 0, 1)));
}

glm::vec3 Transform::GetWorldRight() const
{
    return glm::normalize(LocalToWorldVector(glm::vec3(1, 0, 0)));
}

glm::vec3 Transform::GetWorldUp() const
{
    return glm::normalize(LocalToWorldVector(glm::vec3(0, 1, 0)));
}

void Transform::SetLocalForward(const glm::vec3& inVec)
{
    m_LocalRotation = GetRotationQuaternionFromTo(inVec, glm::vec3(0, 0, 1));
}

void Transform::SetLocalRight(const glm::vec3& inVec)
{
    m_LocalRotation = GetRotationQuaternionFromTo(inVec, glm::vec3(1, 0, 0));
}

void Transform::SetLocalUp(const glm::vec3& inVec)
{
    m_LocalRotation = GetRotationQuaternionFromTo(inVec, glm::vec3(0, 1, 0));
}

void Transform::SetWorldForward(const glm::vec3& inVec)
{
    const glm::vec3 localForward = WorldToLocalDirection(inVec);
    SetLocalForward(localForward);
}

void Transform::SetWorldRight(const glm::vec3& inVec)
{
    const glm::vec3 localRight = WorldToLocalDirection(inVec);
    SetLocalRight(localRight);
}

void Transform::SetWorldUp(const glm::vec3& inVec)
{
    const glm::vec3 localUp = WorldToLocalDirection(inVec);
    SetLocalUp(localUp);
}

glm::vec3 Transform::RotateVector(const glm::quat& inQuat, const glm::vec3& inVec)
{
    glm::quat q = inQuat;
    glm::quat p = glm::quat(0, inVec.x, inVec.y, inVec.z);
    glm::quat result = q * p * glm::conjugate(q);
    return {result.x, result.y, result.z};
}

glm::mat3 Transform::GetRotationMatrixFromTo(const glm::vec3& from, const glm::vec3& to)
{
    glm::vec3 v = glm::cross(from, to);
    float e = glm::dot(from, to);
    const float epsilon = 0.000001f;
    glm::mat3 m;
    if (e > 1.0 - epsilon)     /* "from" almost or equal to "to"-vector? */
    {
        /* return identity matrix */
        m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f;
        m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f;
        m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f;
    }
    else if (e < -1.0 + epsilon) /* "from" almost or equal to negated "to"? */
    {
        float invlen;
        float fxx, fyy, fzz, fxy, fxz, fyz;
        float uxx, uyy, uzz, uxy, uxz, uyz;
        float lxx, lyy, lzz, lxy, lxz, lyz;
        /* left=CROSS(from, (1,0,0)) */
        glm::vec3 left(0.0f, from[2], -from[1]);
        if (glm::dot(left, left) < epsilon) /* was left=CROSS(from,(1,0,0)) a good choice? */
        {
            /* here we now that left = CROSS(from, (1,0,0)) will be a good choice */
            left[0] = -from[2]; left[1] = 0.0; left[2] = from[0];
        }
        /* normalize "left" */
        invlen = 1.0f / std::sqrt(glm::dot(left, left));
        left[0] *= invlen;
        left[1] *= invlen;
        left[2] *= invlen;
        glm::vec3 up = glm::cross(left, from);
        /* now we have a coordinate system, i.e., a basis;    */
        /* M=(from, up, left), and we want to rotate to:      */
        /* N=(-from, up, -left). This is done with the matrix:*/
        /* N*M^T where M^T is the transpose of M              */
        fxx = -from[0] * from[0]; fyy = -from[1] * from[1]; fzz = -from[2] * from[2];
        fxy = -from[0] * from[1]; fxz = -from[0] * from[2]; fyz = -from[1] * from[2];

        uxx = up[0] * up[0]; uyy = up[1] * up[1]; uzz = up[2] * up[2];
        uxy = up[0] * up[1]; uxz = up[0] * up[2]; uyz = up[1] * up[2];

        lxx = -left[0] * left[0]; lyy = -left[1] * left[1]; lzz = -left[2] * left[2];
        lxy = -left[0] * left[1]; lxz = -left[0] * left[2]; lyz = -left[1] * left[2];
        /* symmetric matrix */
        m[0][0] = fxx + uxx + lxx;  m[0][1] = fxy + uxy + lxy;  m[0][2] = fxz + uxz + lxz;
        m[1][0] = m[0][1];          m[1][1] = fyy + uyy + lyy;  m[1][2] = fyz + uyz + lyz;
        m[2][0] = m[0][2];          m[2][1] = m[1][2];          m[2][2] = fzz + uzz + lzz;
    }
    else  /* the most common case, unless "from"="to", or "from"=-"to" */
    {
        /* ...otherwise use this hand optimized version (9 mults less) */
        float hvx, hvz, hvxy, hvxz, hvyz;
        float h = (1.0f - e) / glm::dot(v, v);
        hvx = h * v[0];
        hvz = h * v[2];
        hvxy = hvx * v[1];
        hvxz = hvx * v[2];
        hvyz = hvz * v[1];
        m[0][0] = e + hvx * v[0];   m[0][1] = hvxy - v[2];        m[0][2] = hvxz + v[1];
        m[1][0] = hvxy + v[2];      m[1][1] = e + h * v[1] * v[1];  m[1][2] = hvyz - v[0];
        m[2][0] = hvxz - v[1];      m[2][1] = hvyz + v[0];          m[2][2] = e + hvz * v[2];
    }
    return m;
}

glm::quat Transform::GetRotationQuaternionFromTo(const glm::vec3& from, const glm::vec3& to)
{
    glm::mat3 m = GetRotationMatrixFromTo(from, to);
    return glm::quat_cast(m);
}