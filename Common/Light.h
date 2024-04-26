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

struct light
{
    float Intensity;
    glm::vec3 Color;
    Transform Transform;
};

struct DirectionalLight : light
{
    glm::vec3 Direction;
};

struct PointLight : light
{
    float Radius;
};

struct SpotLight : light
{
    glm::vec3 Direction;
    float Radius;
    float Angle;
};