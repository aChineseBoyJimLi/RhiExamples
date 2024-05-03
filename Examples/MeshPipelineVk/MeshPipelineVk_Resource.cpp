#include "MeshPipelineVk.h"
#include <random>

bool MeshPipelineVk::CreateDepthStencilBuffer()
{
    if(!CreateTexture(m_Width
        , m_Height
        , s_DepthStencilFormat
        , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , VK_IMAGE_LAYOUT_UNDEFINED
        , m_DepthStencilTexture
        , m_DepthStencilMemory))
    {
        Log::Error("Failed to create depth stencil texture");
        return false;
    }

    if(!CreateImageView(m_DepthStencilTexture
        , s_DepthStencilFormat
        , VK_IMAGE_ASPECT_DEPTH_BIT
        , m_Dsv))
    {
        Log::Error("Failed to create depth stencil view");
        return false;
    }
    
    return true;
}

void MeshPipelineVk::DestroyDepthStencilBuffer()
{
    if(m_Dsv != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_DeviceHandle, m_Dsv, nullptr);
        m_Dsv = VK_NULL_HANDLE;
    }

    if(m_DepthStencilMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_DepthStencilMemory, nullptr);
        m_DepthStencilMemory = VK_NULL_HANDLE;
    }
    
    if(m_DepthStencilTexture != VK_NULL_HANDLE)
    {
        vkDestroyImage(m_DeviceHandle, m_DepthStencilTexture, nullptr);
        m_DepthStencilTexture = VK_NULL_HANDLE;
    }
}

bool MeshPipelineVk::CreateFrameBuffer()
{
    m_FrameBuffers.resize(s_BackBufferCount);

    for(uint32_t i = 0; i < s_BackBufferCount; ++i)
    {
        VkImageView attachments[] = { m_BackBufferViews[i], m_Dsv };

        VkFramebufferCreateInfo frameBufferInfo = {};
        frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferInfo.renderPass = m_RenderPassHandle;
        frameBufferInfo.attachmentCount = 2;
        frameBufferInfo.pAttachments = attachments;
        frameBufferInfo.width = m_Width;
        frameBufferInfo.height = m_Height;
        frameBufferInfo.layers = 1;

        if(vkCreateFramebuffer(m_DeviceHandle, &frameBufferInfo, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS)
        {
            Log::Error("Failed to create frame buffer");
            return false;
        }
    }
    
    return true;
}

void MeshPipelineVk::DestroyFrameBuffer()
{
    for(auto i : m_FrameBuffers)
    {
        if(i != VK_NULL_HANDLE)
            vkDestroyFramebuffer(m_DeviceHandle, i, nullptr);
    }
    m_FrameBuffers.clear();
}

bool MeshPipelineVk::CreateScene()
{
    m_Camera.AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    m_Camera.Transform.SetWorldPosition(glm::vec3(0, 20, 0));
    m_Camera.Transform.LookAt(glm::vec3(0, 0, 0));
    
    m_Mesh = AssetsManager::LoadMeshImmediately("sphere.fbx");
    if(m_Mesh == nullptr || m_Mesh->IsEmpty())
        return false;

    std::vector<DirectX::CullData> cullData;
    if(!m_Mesh->ComputeMeshlets(m_Meshlets
        , m_UniqueVertexIndices
        , m_PackedPrimitiveIndices
        , cullData))
    {
        return false;
    }

    for(auto i : cullData)
    {
        m_MeshletCullData.push_back(glm::vec4(i.BoundingSphere.Center.x, i.BoundingSphere.Center.y, i.BoundingSphere.Center.z, i.BoundingSphere.Radius));
    }

    m_Mesh->GetPositionData(m_PositionData);
    m_Mesh->GetTexCoord0Data(m_TexCoord0Data);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis01(0, 1.0f);
    std::uniform_real_distribution<float> dis11(-1.0f, 1.0f);

    glm::vec3 zoom(s_InstanceCountX * 2.5f, s_InstanceCountY * 2.5f, s_InstanceCountZ * 2.5f) ;
    
    for(uint32_t x = 0; x < s_InstanceCountX; x++)
    {
        for(uint32_t y = 0; y < s_InstanceCountY; y++)
        {
            for(uint32_t z = 0; z < s_InstanceCountZ; z++)
            {
                uint32_t i = x * s_InstanceCountY * s_InstanceCountZ + y * s_InstanceCountZ + z;
                Transform transform;
                glm::vec3 pos((float)x / (float)s_InstanceCountX, (float)y / (float)s_InstanceCountY, (float)z / (float)s_InstanceCountZ);
                transform.SetWorldPosition(glm::mix(-zoom, zoom, pos));
                transform.SetLocalRotation(glm::vec3(dis11(rd), dis11(rd), dis11(rd)));
                
                m_InstancesData[i].LocalToWorld = transform.GetLocalToWorldMatrix();
                m_InstancesData[i].WorldToLocal = transform.GetWorldToLocalMatrix();
            }
        }
    }

    uint32_t totalMeshletCount = s_InstancesCount * (uint32_t)m_Meshlets.size();
    m_GroupCount = totalMeshletCount / s_ASThreadGroupSize;
    if(totalMeshletCount % s_ASThreadGroupSize != 0)
    {
        m_GroupCount += 1;;
    }

    m_MeshInfo.IndexCount = m_Mesh->GetIndicesCount();
    m_MeshInfo.VertexCount = m_Mesh->GetVerticesCount();
    m_MeshInfo.MeshletCount = (uint32_t)m_Meshlets.size();
    m_MeshInfo.InstanceCount = s_InstancesCount;

    return true;
}

bool MeshPipelineVk::CreateResources()
{
    if(!CreateBuffer(CameraData::GetAlignedByteSizes()
        , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        , m_CameraDataBuffer
        , m_CameraBufferMemory))
    {
        Log::Error("Failed to create camera data buffer");
        return false;
    }

    if(!CreateBuffer(ViewFrustumCB::GetAlignedByteSizes()
        , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        , m_ViewFrustumBuffer
        , m_ViewFrustumBufferMemory))
    {
        Log::Error("Failed to create view frustum buffer");
        return false;
    }

    if(!CreateBuffer(MeshInfo::GetAlignedByteSizes()
        , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        , m_MeshInfoBuffer
        , m_MeshInfoBufferMemory))
    {
        Log::Error("Failed to create mesh info buffer");
        return false;
    }

    WriteBufferData(m_DeviceHandle, m_MeshInfoBufferMemory, &m_MeshInfo, MeshInfo::GetAlignedByteSizes());
    
    if(!CreateBuffer(m_PositionData.size() * sizeof(glm::vec4)
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_VerticesBuffer
        , m_VerticesBufferMemory))
    {
        Log::Error("Failed to create vertex buffer");
        return false;
    }

    if(!CreateBuffer(m_TexCoord0Data.size() * sizeof(glm::vec2)
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_TexCoordsBuffer
        , m_TexCoordsBufferMemory))
    {
        Log::Error("Failed to create index buffer");
        return false;
    }

    if(!CreateBuffer(m_Meshlets.size() * sizeof(DirectX::Meshlet)
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_MeshletsBuffer
        , m_MeshletsBufferMemory))
    {
        Log::Error("Failed to create meshlets buffer");
        return false;
    }

    if(!CreateBuffer(m_PackedPrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle)
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_PackedPrimitiveIndicesBuffer
        , m_PackedPrimitiveIndicesBufferMemory))
    {
        Log::Error("Failed to create packed primitive indices buffer");
        return false;
    }

    if(!CreateBuffer(m_UniqueVertexIndices.size() * sizeof(uint8_t)
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_UniqueVertexIndicesBuffer
        , m_UniqueVertexIndicesBufferMemory))
    {
        Log::Error("Failed to create unique vertex indices buffer");
        return false;
    }

    if(!CreateBuffer(m_MeshletCullData.size() * sizeof(glm::vec4)
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_MeshletCullDataBuffer
        , m_MeshletCullDataBufferMemory))
    {
        Log::Error("Failed to create meshlet cull data buffer");
        return false;
    }
    
    const size_t instanceBufferBytesSize = s_InstancesCount * sizeof(InstanceData);
    if(!CreateBuffer(instanceBufferBytesSize
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_InstanceBuffer
        , m_InstanceBufferMemory))
    {
        Log::Error("Failed to create instance buffer");
        return false;
    }
    

    std::vector<std::shared_ptr<StagingBuffer>> stagingBuffers(7);

    BeginCommandList();

    stagingBuffers[0] = UploadBuffer(m_VerticesBuffer, m_PositionData.data(), m_PositionData.size() * sizeof(glm::vec4));
    stagingBuffers[1] = UploadBuffer(m_TexCoordsBuffer, m_TexCoord0Data.data(), m_TexCoord0Data.size() * sizeof(glm::vec2));
    stagingBuffers[2] = UploadBuffer(m_MeshletsBuffer, m_Meshlets.data(), m_Meshlets.size() * sizeof(DirectX::Meshlet));
    stagingBuffers[3] = UploadBuffer(m_PackedPrimitiveIndicesBuffer, m_PackedPrimitiveIndices.data(), m_PackedPrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle));
    stagingBuffers[4] = UploadBuffer(m_UniqueVertexIndicesBuffer, m_UniqueVertexIndices.data(), m_UniqueVertexIndices.size() * sizeof(uint8_t));
    stagingBuffers[5] = UploadBuffer(m_MeshletCullDataBuffer, m_MeshletCullData.data(), m_MeshletCullData.size() * sizeof(glm::vec4));
    stagingBuffers[6] = UploadBuffer(m_InstanceBuffer, m_InstancesData.data(), instanceBufferBytesSize);
    
    EndCommandList();

    ExecuteCommandBuffer();
    
    return true;
}

void MeshPipelineVk::DestroyResources()
{
    if(m_InstanceBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_InstanceBufferMemory, nullptr);
        m_InstanceBufferMemory = VK_NULL_HANDLE;
    }
    if(m_InstanceBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_InstanceBuffer, nullptr);
        m_InstanceBuffer = VK_NULL_HANDLE;
    }

    if(m_MeshletCullDataBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_MeshletCullDataBufferMemory, nullptr);
        m_MeshletCullDataBufferMemory = VK_NULL_HANDLE;
    }
    if(m_MeshletCullDataBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_MeshletCullDataBuffer, nullptr);
        m_MeshletCullDataBuffer = VK_NULL_HANDLE;
    }

    if(m_UniqueVertexIndicesBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_UniqueVertexIndicesBufferMemory, nullptr);
        m_UniqueVertexIndicesBufferMemory = VK_NULL_HANDLE;
    }
    if(m_UniqueVertexIndicesBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_UniqueVertexIndicesBuffer, nullptr);
        m_UniqueVertexIndicesBuffer = VK_NULL_HANDLE;
    }
    
    if(m_PackedPrimitiveIndicesBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_PackedPrimitiveIndicesBufferMemory, nullptr);
        m_PackedPrimitiveIndicesBufferMemory = VK_NULL_HANDLE;
    }
    if(m_PackedPrimitiveIndicesBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_PackedPrimitiveIndicesBuffer, nullptr);
        m_PackedPrimitiveIndicesBuffer = VK_NULL_HANDLE;
    }
    
    if(m_MeshletsBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_MeshletsBufferMemory, nullptr);
        m_MeshletsBufferMemory = VK_NULL_HANDLE;
    }
    
    if(m_MeshletsBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_MeshletsBuffer, nullptr);
        m_MeshletsBuffer = VK_NULL_HANDLE;
    }
    
    if(m_TexCoordsBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_TexCoordsBufferMemory, nullptr);
        m_TexCoordsBufferMemory = VK_NULL_HANDLE;
    }
    if(m_TexCoordsBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_TexCoordsBuffer, nullptr);
        m_TexCoordsBuffer = VK_NULL_HANDLE;
    }
    if(m_VerticesBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_VerticesBufferMemory, nullptr);
        m_VerticesBufferMemory = VK_NULL_HANDLE;
    }
    if(m_VerticesBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_VerticesBuffer, nullptr);
        m_VerticesBuffer = VK_NULL_HANDLE;
    }
    if(m_MeshInfoBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_MeshInfoBufferMemory, nullptr);
        m_MeshInfoBufferMemory = VK_NULL_HANDLE;
    }
    if(m_MeshInfoBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_MeshInfoBuffer, nullptr);
        m_MeshInfoBuffer = VK_NULL_HANDLE;
    }

    if(m_ViewFrustumBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_ViewFrustumBufferMemory, nullptr);
        m_ViewFrustumBufferMemory = VK_NULL_HANDLE;
    }

    if(m_ViewFrustumBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_ViewFrustumBuffer, nullptr);
        m_ViewFrustumBuffer = VK_NULL_HANDLE;
    }
    
    if(m_CameraBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_CameraBufferMemory, nullptr);
        m_CameraBufferMemory = VK_NULL_HANDLE;
    }
    if(m_CameraDataBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_CameraDataBuffer, nullptr);
        m_CameraDataBuffer = VK_NULL_HANDLE;
    }
}

bool MeshPipelineVk::CreateDescriptorSet()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo {};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = m_DescriptorPoolHandle;
    descriptorSetAllocInfo.pSetLayouts = &m_DescriptorLayout;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    if(vkAllocateDescriptorSets(m_DeviceHandle, &descriptorSetAllocInfo, &m_DescriptorSet) != VK_SUCCESS)
    {
        Log::Error("Failed to allocate descriptor set");
        return false;
    }

    std::array<VkWriteDescriptorSet, 10> descriptorWrites{};
    VkDescriptorBufferInfo cameraBufferInfo = CreateDescriptorBufferInfo(m_CameraDataBuffer, CameraData::GetAlignedByteSizes());
    UpdateBufferDescriptor(descriptorWrites[0], m_DescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &cameraBufferInfo, GetBindingSlot(ERegisterType::ConstantBuffer, 0)); // _CameraData 
    
    VkDescriptorBufferInfo viewFrustumInfo = CreateDescriptorBufferInfo(m_ViewFrustumBuffer, ViewFrustumCB::GetAlignedByteSizes());
    UpdateBufferDescriptor(descriptorWrites[1], m_DescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &viewFrustumInfo, GetBindingSlot(ERegisterType::ConstantBuffer, 1)); // _ViewFrustum

    VkDescriptorBufferInfo meshInfoBufferInfo = CreateDescriptorBufferInfo(m_MeshInfoBuffer, MeshInfo::GetAlignedByteSizes());
    UpdateBufferDescriptor(descriptorWrites[2], m_DescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &meshInfoBufferInfo, GetBindingSlot(ERegisterType::ConstantBuffer, 2)); // _MeshInfo


    VkDescriptorBufferInfo verticesBufferInfo = CreateDescriptorBufferInfo(m_VerticesBuffer, m_PositionData.size() * sizeof(glm::vec4));
    UpdateBufferDescriptor(descriptorWrites[3], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &verticesBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 0)); // _Vertices

    VkDescriptorBufferInfo texCoordsBufferInfo = CreateDescriptorBufferInfo(m_TexCoordsBuffer, m_TexCoord0Data.size() * sizeof(glm::vec2));
    UpdateBufferDescriptor(descriptorWrites[4], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &texCoordsBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 1)); // _TexCoords

    VkDescriptorBufferInfo meshletsBufferInfo = CreateDescriptorBufferInfo(m_MeshletsBuffer, m_Meshlets.size() * sizeof(DirectX::Meshlet));
    UpdateBufferDescriptor(descriptorWrites[5], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &meshletsBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 2)); // _Meshlets

    VkDescriptorBufferInfo packedPrimitiveIndicesBufferInfo = CreateDescriptorBufferInfo(m_PackedPrimitiveIndicesBuffer, m_PackedPrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle));
    UpdateBufferDescriptor(descriptorWrites[6], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &packedPrimitiveIndicesBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 3)); // _PackedPrimitiveIndices

    VkDescriptorBufferInfo uniqueVertexIndicesBufferInfo = CreateDescriptorBufferInfo(m_UniqueVertexIndicesBuffer, m_UniqueVertexIndices.size() * sizeof(uint8_t));
    UpdateBufferDescriptor(descriptorWrites[7], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &uniqueVertexIndicesBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 4)); // _UniqueVertexIndices

    VkDescriptorBufferInfo meshletCullDataBufferInfo = CreateDescriptorBufferInfo(m_MeshletCullDataBuffer, m_MeshletCullData.size() * sizeof(glm::vec4));
    UpdateBufferDescriptor(descriptorWrites[8], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &meshletCullDataBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 5)); // _MeshletCullData

    VkDescriptorBufferInfo instanceBufferInfo = CreateDescriptorBufferInfo(m_InstanceBuffer, s_InstancesCount * sizeof(InstanceData));
    UpdateBufferDescriptor(descriptorWrites[9], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &instanceBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 6)); // _InstanceData
    
    vkUpdateDescriptorSets(m_DeviceHandle, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    
    return true;
}

void MeshPipelineVk::DestroyDescriptorSet()
{
    if(m_DescriptorSet != VK_NULL_HANDLE)
    {
        vkFreeDescriptorSets(m_DeviceHandle, m_DescriptorPoolHandle, 1, &m_DescriptorSet);
        m_DescriptorSet = VK_NULL_HANDLE;
    }
}