#pragma once
#include "Transform.h"

struct DirectionalLightData
{
    glm::mat4   WorldToLight;
    glm::mat4   LightToWorld;
    glm::mat4   LightProjection;
    glm::vec3   LightDirection;
    float       LightIntensity;
    glm::vec3   LightColor;

    static uint32_t GetAlignedByteSizes()
    {
        return (sizeof(DirectionalLightData) + 255) & ~255;
    }
};

struct Light
{
    Transform Transform;
    float Intensity;
    glm::vec3 Color;
};

struct PointLight : Light
{
    float Radius;
};

struct SpotLight : Light
{
    glm::vec3 Direction;
    float Radius;
    float Angle;
};