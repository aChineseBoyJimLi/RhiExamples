#pragma once

#include "../AppBaseVk.h"
#include "Transform.h"
#include "Camera.h"
#include "Light.h"
#include "AssetsManager.h"
#include <array>

struct MeshInfo
{
    uint32_t VertexCount;
    uint32_t IndexCount;
    uint32_t MeshletCount;
    uint32_t InstanceCount;

    static uint64_t GetAlignedByteSizes()
    {
        return (sizeof(MeshInfo) + 255) & ~255;
    }
};

struct InstanceData
{
    glm::mat4 LocalToWorld;
    glm::mat4 WorldToLocal;
};

struct ViewFrustumCB
{
    glm::vec4 Corners[8];
    static uint64_t GetAlignedByteSizes()
    {
        return (sizeof(ViewFrustumCB) + 255) & ~255;
    }
};

class MeshPipelineVk : public AppBaseVk
{
public:
    using AppBaseVk::AppBaseVk;
    static constexpr uint32_t s_InstanceCountX = 8, s_InstanceCountY = 8, s_InstanceCountZ = 8;
    static constexpr uint32_t s_InstancesCount = s_InstanceCountX * s_InstanceCountY * s_InstanceCountZ;
    static constexpr uint32_t s_ASThreadGroupSize = 32;
    static constexpr uint32_t s_MaterialCount = 100;
    static constexpr VkFormat s_DepthStencilFormat = VK_FORMAT_D32_SFLOAT;

protected:
    bool Init() override;
    void Tick() override;
    void Shutdown() override;

private:
    bool CreateDevice() override;
    void DestroyDevice() override;
    bool CreateDescriptorLayout();
    void DestroyDescriptorLayout();
    bool CreateShader();
    void DestroyShader();
    bool CreateRenderPass();
    void DestroyRenderPass();
    bool CreatePipelineState();
    void DestroyPipelineState();
    bool CreateDepthStencilBuffer();
    void DestroyDepthStencilBuffer();
    bool CreateFrameBuffer();
    void DestroyFrameBuffer();
    bool CreateScene();
    bool CreateResources();
    void DestroyResources();
    bool CreateDescriptorSet();
    void DestroyDescriptorSet();
    void UpdateConstants();

    PFN_vkCmdDrawMeshTasksEXT               vkCmdDrawMeshTasksEXT;
    
    VkDescriptorSetLayout                   m_DescriptorLayout;
    std::shared_ptr<AssetsManager::Blob>    m_ASBlob;
    std::shared_ptr<AssetsManager::Blob>    m_MSBlob;
    std::shared_ptr<AssetsManager::Blob>    m_PSBlob;
    VkShaderModule                          m_ASModule;
    VkShaderModule                          m_MSModule;
    VkShaderModule                          m_PSModule;
    VkRenderPass                            m_RenderPassHandle;
    VkPipelineLayout                        m_PipelineLayout;
    VkPipeline                              m_PipelineState;

    VkDeviceMemory                          m_DepthStencilMemory;
    VkImage                                 m_DepthStencilTexture;
    VkImageView                             m_Dsv;
    std::vector<VkFramebuffer>              m_FrameBuffers;

    CameraPerspective                                   m_Camera;
    MeshInfo                                            m_MeshInfo;
    std::vector<glm::vec4>                              m_PositionData;
    std::vector<glm::vec2>                              m_TexCoord0Data;
    std::shared_ptr<AssetsManager::Mesh>                m_Mesh;
    std::vector<DirectX::Meshlet>                       m_Meshlets;
    std::vector<uint8_t>                                m_UniqueVertexIndices;
    std::vector<DirectX::MeshletTriangle>               m_PackedPrimitiveIndices;
    std::vector<glm::vec4>                              m_MeshletCullData;
    std::array<InstanceData, s_InstancesCount>          m_InstancesData;
    uint32_t                                            m_GroupCount;
    
    VkBuffer                    m_CameraDataBuffer;
    VkDeviceMemory              m_CameraBufferMemory;
    VkBuffer                    m_ViewFrustumBuffer;
    VkDeviceMemory              m_ViewFrustumBufferMemory;
    VkBuffer                    m_MeshInfoBuffer;
    VkDeviceMemory              m_MeshInfoBufferMemory;

    VkBuffer                    m_VerticesBuffer;
    VkDeviceMemory              m_VerticesBufferMemory;
    VkBuffer                    m_TexCoordsBuffer;
    VkDeviceMemory              m_TexCoordsBufferMemory;
    VkBuffer                    m_MeshletsBuffer;
    VkDeviceMemory              m_MeshletsBufferMemory;
    VkBuffer                    m_PackedPrimitiveIndicesBuffer;
    VkDeviceMemory              m_PackedPrimitiveIndicesBufferMemory;
    VkBuffer                    m_UniqueVertexIndicesBuffer;
    VkDeviceMemory              m_UniqueVertexIndicesBufferMemory;
    VkBuffer                    m_MeshletCullDataBuffer;
    VkDeviceMemory              m_MeshletCullDataBufferMemory;
    VkBuffer                    m_InstanceBuffer;
    VkDeviceMemory              m_InstanceBufferMemory;

    VkDescriptorSet             m_DescriptorSet;
};