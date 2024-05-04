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
    uint32_t TextureIndex;
    uint32_t IsLambertian; // 0: Mirror reflection, 1: Lambertian
    float Padding0;
    float Padding1;
};

struct AABB
{
    float MinX;
    float MinY;
    float MinZ;
    float MaxX;
    float MaxY;
    float MaxZ; 
};

class RayTracingPipelineVk : public AppBaseVk
{
public:
    using AppBaseVk::AppBaseVk;
    static constexpr uint32_t           s_MaxRayRecursionDepth = 3;
    static constexpr uint32_t           s_TriangleMeshInstanceCount = 2;
    static constexpr uint32_t           s_AABBMeshInstanceCount = 1;
    static constexpr uint32_t           s_InstanceCount = s_TriangleMeshInstanceCount + s_AABBMeshInstanceCount;
    static constexpr uint32_t           s_MaterialCount = 2;
    
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

    VkDeviceAddress GetBufferDeviceAddress(VkBuffer inBuffer);
    VkDeviceAddress GetAccelerationStructureDeviceAddress(VkAccelerationStructureKHR inAccelerationStructure);

    void BuildAccelerationStructure(VkAccelerationStructureTypeKHR inType
        , VkAccelerationStructureKHR inAcclerationStructure
        , VkBuffer inScratchBuffer
        , const VkAccelerationStructureGeometryKHR* inGeometry
        , uint32_t inNumPrimitives);
    
    bool CreateDevice() override;
    void DestroyDevice() override;
    bool CreateDescriptorLayout();
    void DestroyDescriptorLayout();
    bool CreateShader();
    void DestroyShader();
    bool CreateShaderTable();
    void DestroyShaderTable();
    bool CreatePipelineState();
    void DestroyPipelineState();
    bool CreateScene();
    bool CreateResources();
    void DestroyResources();
    bool CreateBottomLevelAccelStructure();
    void DestroyBottomLevelAccelStructure();
    bool CreateTopLevelAccelStructure();
    void DestroyTopLevelAccelStructure();
    bool CreateDescriptorSet();
    void DestroyDescriptorSet();
    void UpdateConstants();
    
    VkDescriptorSetLayout                               m_DescriptorLayout;
    VkPipelineLayout                                    m_PipelineLayout;
    VkPipeline                                          m_PipelineState;
    std::shared_ptr<AssetsManager::Blob>                m_RayGenShaderBlob;
    std::shared_ptr<AssetsManager::Blob>                m_MissShadersBlob;
    std::shared_ptr<AssetsManager::Blob>                m_HitGroupShadersBlob;
    VkShaderModule                                      m_RayGenShaderModule;
    VkShaderModule                                      m_MissShaderModule;
    VkShaderModule                                      m_ClosestHitShaderModule;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR>   m_ShaderGroups;
    VkBuffer                                            m_ShaderTableBuffer;
    VkDeviceMemory                                      m_ShaderTableBufferMemory;
    VkStridedDeviceAddressRegionKHR                     m_RaygenShaderSbtEntry{};
    VkStridedDeviceAddressRegionKHR                     m_MissShaderSbtEntry{};
    VkStridedDeviceAddressRegionKHR                     m_HitShaderSbtEntry{};
    VkStridedDeviceAddressRegionKHR                     m_CallableShaderSbtEntry{};
    
    CameraPerspective                                   m_Camera;
    Light                                               m_MainLight;    
    std::shared_ptr<AssetsManager::Mesh>                m_Mesh;
    std::vector<glm::vec2>                              m_TexCoord0Data;
    std::vector<glm::vec4>                              m_NormalData;
    std::array<Transform, s_InstanceCount>              m_InstancesTransform;
    std::array<InstanceData, s_InstanceCount>           m_InstancesData;
    std::array<MaterialData, s_MaterialCount>           m_MaterialsData;
    std::array<AABB, s_AABBMeshInstanceCount>           m_AABB;
    

    VkImage                     m_OutputImage;
    VkDeviceMemory              m_OutputImageMemory;
    VkImageView                 m_OutputImageView;

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
    VkBuffer                    m_TexcoordsBuffer;
    VkDeviceMemory              m_TexcoordsBufferMemory;
    VkBuffer                    m_NormalsBuffer;
    VkDeviceMemory              m_NormalsBufferMemory;
    VkBuffer                    m_AABBBuffer;
    VkDeviceMemory              m_AABBBufferMemory;

    VkBuffer                    m_BLASBuffer;
    VkDeviceMemory              m_BLASBufferMemory;
    VkAccelerationStructureKHR  m_BLAS;
    VkBuffer                    m_ProceduralGeoBLASBuffer;
    VkDeviceMemory              m_ProceduralGeoBLASBufferMemory;
    VkAccelerationStructureKHR  m_ProceduralGeoBLAS;
    VkBuffer                    m_TLASBuffer;
    VkDeviceMemory              m_TLASBufferMemory;
    VkAccelerationStructureKHR  m_TLAS;
    VkDescriptorSet             m_DescriptorSet;
};