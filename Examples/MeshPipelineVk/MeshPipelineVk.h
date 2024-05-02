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

struct AABB
{
    glm::vec4 Min;
    glm::vec4 Max;
    static uint64_t GetAlignedByteSizes()
    {
        return (sizeof(AABB) + 255) & ~255;
    }
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
    static constexpr uint32_t s_TexturesCount = 5;
    // static constexpr uint32_t s_InstanceCountX = 16, s_InstanceCountY = 16, s_InstanceCountZ = 16;
    static constexpr uint32_t s_InstanceCountX = 8, s_InstanceCountY = 8, s_InstanceCountZ = 8;
    static constexpr uint32_t s_InstancesCount = s_InstanceCountX * s_InstanceCountY * s_InstanceCountZ;
    static constexpr uint32_t s_ThreadGroupSize = 128;
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
    
    VkDescriptorSetLayout                   m_DescriptorLayout;
    VkDescriptorSetLayout                   m_DescriptorLayoutSpace1;
    VkDescriptorSetLayout                   m_CullingPassDescriptorSetLayout;
    std::shared_ptr<AssetsManager::Blob>    m_VertexShaderBlob;
    std::shared_ptr<AssetsManager::Blob>    m_PixelShaderBlob;
    std::shared_ptr<AssetsManager::Blob>    m_ComputeShaderBlob;
    VkShaderModule                          m_VertexShaderModule;
    VkShaderModule                          m_PixelShaderModule;
    VkShaderModule                          m_ComputeShaderModule;
    VkRenderPass                            m_RenderPassHandle;
    VkPipelineLayout                        m_PipelineLayout;
    VkPipelineLayout                        m_CullingPassPipelineLayout;
    VkPipeline                              m_PipelineState;
    VkPipeline                              m_CullingPassPipelineState;

    VkDeviceMemory              m_DepthStencilMemory;
    VkImage                     m_DepthStencilTexture;
    VkImageView                 m_Dsv;

    std::vector<VkFramebuffer>  m_FrameBuffers;

    CameraPerspective                                   m_Camera;
    Light                                               m_Light;    
    std::shared_ptr<AssetsManager::Mesh>                m_Mesh;
    std::array<std::shared_ptr<AssetsManager::Texture>, s_TexturesCount>  m_Textures;
    std::vector<VertexData>     m_VerticesData;
    std::array<InstanceData, s_InstancesCount> m_InstancesData;
    std::array<MaterialData, s_MaterialCount> m_MaterialsData;

    VkBuffer                    m_CameraDataBuffer;
    VkDeviceMemory              m_CameraBufferMemory;
    VkBuffer                    m_ViewFrustumBuffer;
    VkDeviceMemory              m_ViewFrustumBufferMemory;
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
    VkBuffer                    m_AABBBuffer;
    VkDeviceMemory              m_AABBBufferMemory;
    VkBuffer                    m_IndirectCommandsBuffer;
    VkDeviceMemory              m_IndirectCommandsBufferMemory;
    
    std::array<VkImage,  s_TexturesCount>           m_MainTextures;
    std::array<VkImageView, s_TexturesCount>        m_MainTextureViews;
    std::array<VkDeviceMemory, s_TexturesCount>     m_MainTextureMemories;
    std::array<VkSampler, s_TexturesCount>     m_MainTextureSamplers;

    std::array<VkDrawIndexedIndirectCommand, s_InstancesCount> m_IndirectDrawCommands;
    
    VkDescriptorSet             m_DescriptorSet;
    VkDescriptorSet             m_CullingPassDescriptorSet;
};