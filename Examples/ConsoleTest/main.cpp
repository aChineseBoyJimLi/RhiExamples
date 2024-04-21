#include "Log.h"
#include "Transform.h"

void LogVector(const glm::vec3& inVec)
{
    Log::Info("Vector: %f, %f, %f", inVec.x, inVec.y, inVec.z);
}

glm::vec3 RotateVector2(const glm::quat& inQuat, const glm::vec3& inVec)
{
    glm::mat4 mat = glm::mat4_cast(inQuat); 
    glm::vec4 vec4(inVec.x, inVec.y, inVec.z, 1.0f); 
    vec4 = mat * vec4; 
    return {vec4.x, vec4.y, vec4.z};
}

int main()
{
    glm::vec3 v(0.1f, 0.45f, 0.9f);
    v = normalize(v);
    LogVector(v);

    glm::quat q = glm::angleAxis(glm::radians(32.15f), glm::vec3(0, 1, 0));
    
    glm::vec3 v1 = RotateVector(q, v);
    LogVector(v1);

    
    glm::vec3 v2 = RotateVector2(q, v);
    LogVector(v2);

    return 0;
}