#pragma once

#include "../AppBaseVk.h"

#include "Transform.h"
#include "Camera.h"
#include "Light.h"
#include "AssetsManager.h"
#include <array>

struct InstanceData
{
    glm::mat4 LocalToWorld;
    glm::mat4 WorldToLocal;
    uint32_t MaterialIndex;
    uint32_t Padding0;
    uint32_t Padding1;
    uint32_t Padding2;
};

struct MaterialData
{
    glm::vec4 Color;
    float Smooth;
    uint32_t TexIndex;
    uint32_t Padding0;
    uint32_t Padding1;
};

struct VertexData
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

class RayTracingPipelineVk : public AppBaseVk
{
public:
    using AppBaseVk::AppBaseVk;
    static constexpr uint32_t s_TexturesCount = 5;
    static constexpr uint32_t s_InstancesCount = 1024;
    static constexpr uint32_t s_MaterialCount = 100;
    static constexpr VkFormat s_DepthStencilFormat = VK_FORMAT_D32_SFLOAT;
    
protected:
    bool Init() override;
    void Tick() override;
    void Shutdown() override;

private:
    PFN_vkGetBufferDeviceAddressKHR                 vkGetBufferDeviceAddressKHR;
    PFN_vkCreateAccelerationStructureKHR            vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR           vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR     vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR  vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR         vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR            vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR                           vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR        vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR              vkCreateRayTracingPipelinesKHR;
    
    bool CreateDevice() override;
    void DestroyDevice() override;
    bool CreateDescriptorLayout();
    void DestroyDescriptorLayout();
    bool CreateShader();
    void DestroyShader();

    bool CreatePipelineState();
    void DestroyPipelineState();

    bool CreateScene();
    bool CreateResources();
    void DestroyResources();
    bool CreateDescriptorSet();
    void DestroyDescriptorSet();
    void UpdateConstants();
    
    VkDescriptorSetLayout                   m_DescriptorLayout;
    VkPipelineLayout                        m_PipelineLayout;
    VkPipeline                              m_PipelineState;
    
    CameraPerspective                                                       m_Camera;
    Light                                                                   m_Light;    
    std::shared_ptr<AssetsManager::Mesh>                                    m_Mesh;
    std::array<std::shared_ptr<AssetsManager::Texture>, s_TexturesCount>    m_Textures;
    std::vector<VertexData>                                                 m_VerticesData;
    std::array<InstanceData, s_InstancesCount>                              m_InstancesData;
    std::array<MaterialData, s_MaterialCount>                               m_MaterialsData;

    VkImage                     m_OutputImage;
    VkDeviceMemory              m_OutputImageMemory;

    VkBuffer                    m_CameraDataBuffer;
    VkDeviceMemory              m_CameraBufferMemory;
    VkBuffer                    m_LightDataBuffer;
    VkDeviceMemory              m_LightDataMemory;
    VkBuffer                    m_InstanceBuffer;
    VkDeviceMemory              m_InstanceBufferMemory;
    VkBuffer                    m_MaterialsBuffer;
    VkDeviceMemory              m_MaterialsBufferMemory;
    VkBuffer                    m_VerticesBuffer;
    VkDeviceMemory              m_VerticesBufferMemory;
    VkBuffer                    m_IndicesBuffer;
    VkDeviceMemory              m_IndicesBufferMemory;
    std::array<VkImage,  s_TexturesCount>           m_MainTextures;
    std::array<VkImageView, s_TexturesCount>        m_MainTextureViews;
    std::array<VkDeviceMemory, s_TexturesCount>     m_MainTextureMemories;
    std::array<VkSampler, s_TexturesCount>     m_MainTextureSamplers;
    
    VkDescriptorSet             m_DescriptorSet;
    
};