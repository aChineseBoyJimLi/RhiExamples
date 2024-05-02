#include "GraphicsPipelineVk.h"
#include <random>

bool GraphicsPipelineVk::CreateDepthStencilBuffer()
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

void GraphicsPipelineVk::DestroyDepthStencilBuffer()
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

bool GraphicsPipelineVk::CreateFrameBuffer()
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

void GraphicsPipelineVk::DestroyFrameBuffer()
{
    for(auto i : m_FrameBuffers)
    {
        if(i != VK_NULL_HANDLE)
            vkDestroyFramebuffer(m_DeviceHandle, i, nullptr);
    }
    m_FrameBuffers.clear();
}

bool GraphicsPipelineVk::CreateScene()
{
    m_Camera.AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    m_Camera.Transform.SetWorldPosition(glm::vec3(0, 20, 20));
    m_Camera.Transform.LookAt(glm::vec3(0, 0, 0));
    
    m_Light.Transform.SetWorldForward(glm::vec3(-1,-1,-1));
    m_Light.Color = glm::vec3(1, 1, 1);
    m_Light.Intensity = 1.0f;
    
    m_Mesh = AssetsManager::LoadMeshImmediately("sphere.fbx");
    if(m_Mesh == nullptr || m_Mesh->IsEmpty())
        return false;
    
    m_Textures[0] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_01_1024x1024.jpg");
    m_Textures[1] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_02_1024_1024.jpg");
    m_Textures[2] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_03_1024_1024.jpg");
    m_Textures[3] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_04_1024_1024.jpg");
    m_Textures[4] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_05_1024_1024.jpg");

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis01(0, 1.0f);
    std::uniform_real_distribution<float> dis11(-1.0f, 1.0f);
    
    for(uint32_t i = 0; i < s_MaterialCount; ++i)
    {
        // random generate material data
        m_MaterialsData[i].Color = glm::vec4(1, 1, 1, 1);
        m_MaterialsData[i].Smooth = dis01(gen) * 5.0f + 0.5f;
        m_MaterialsData[i].TexIndex = static_cast<uint32_t>(s_TexturesCount * dis01(gen));
    }
    
    for(uint32_t i = 0; i < s_InstancesCount; ++i)
    {
        // random generate instance data
        Transform transform;
        transform.SetWorldPosition(glm::vec3(dis11(rd), dis11(rd), dis11(rd)) * 10.0f);
        float scale = dis01(rd) + 0.001f;
        transform.SetLocalScale(glm::vec3(scale, scale, scale));
        transform.SetLocalRotation(glm::vec3(dis11(rd), dis11(rd), dis11(rd)));
        m_InstancesData[i].LocalToWorld = transform.GetLocalToWorldMatrix();
        m_InstancesData[i].WorldToLocal = transform.GetWorldToLocalMatrix();
        m_InstancesData[i].MaterialIndex = i % s_MaterialCount;
    }
    
    m_VerticesData.resize(m_Mesh->GetVerticesCount());
    const aiMesh* mesh = m_Mesh->GetMesh();
    for(uint32_t i = 0; i < m_Mesh->GetVerticesCount(); ++i)
    {
        m_VerticesData[i].Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        m_VerticesData[i].Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        m_VerticesData[i].TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
    }

    return true;
}

bool GraphicsPipelineVk::CreateResources()
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

    if(!CreateBuffer(DirectionalLightData::GetAlignedByteSizes()
        , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        , m_LightDataBuffer
        , m_LightDataMemory))
    {
        Log::Error("Failed to create light data buffer");
        return false;
    }

    const size_t vertexBufferSize = m_Mesh->GetVerticesCount() * sizeof(VertexData);
    if(!CreateBuffer(vertexBufferSize
        , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_VerticesBuffer
        , m_VerticesBufferMemory))
    {
        Log::Error("Failed to create vertex buffer");
        return false;
    }

    if(!CreateBuffer(m_Mesh->GetIndicesDataByteSize()
        , VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_IndicesBuffer
        , m_IndicesBufferMemory))
    {
        Log::Error("Failed to create index buffer");
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

    const size_t materialsBufferBytesSize = s_MaterialCount * sizeof(MaterialData);
    if(!CreateBuffer(materialsBufferBytesSize
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_MaterialsBuffer
        , m_MaterialsBufferMemory))
    {
        Log::Error("Failed to create materials buffer");
        return false;
    }

    for(uint32_t i = 0; i < s_TexturesCount; ++i)
    {
        const DirectX::TexMetadata& metadata = m_Textures[i]->GetTextureDesc();
        if(!CreateTexture(metadata.width
            , metadata.height
            , VK_FORMAT_B8G8R8A8_UNORM
            , VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            , VK_IMAGE_LAYOUT_UNDEFINED
            , m_MainTextures[i]
            , m_MainTextureMemories[i]))
        {
            Log::Error("Failed to create texture");
            return false;
        }

        if(!CreateImageView(m_MainTextures[i]
            , VK_FORMAT_B8G8R8A8_UNORM
            , VK_IMAGE_ASPECT_COLOR_BIT
            , m_MainTextureViews[i]))
        {
            Log::Error("Failed to create image view");
            return false;
        }

        if(!CreateSampler(m_MainTextureSamplers[i]))
        {
            Log::Error("Failed to create sampler");
            return false;
        }
    }

    std::vector<std::shared_ptr<StagingBuffer>> stagingBuffers(9);

    BeginCommandList();

    stagingBuffers[0] = UploadBuffer(m_VerticesBuffer, m_VerticesData.data(), vertexBufferSize);
    stagingBuffers[1] = UploadBuffer(m_IndicesBuffer, m_Mesh->GetIndicesData(), m_Mesh->GetIndicesDataByteSize());
    stagingBuffers[2] = UploadBuffer(m_InstanceBuffer, m_InstancesData.data(), instanceBufferBytesSize);
    stagingBuffers[3] = UploadBuffer(m_MaterialsBuffer, m_MaterialsData.data(), materialsBufferBytesSize);

    for(uint32_t i = 0; i < s_TexturesCount; ++i)
    {
        const DirectX::ScratchImage& scratchImage = m_Textures[i]->GetScratchImage();
        const DirectX::TexMetadata& metadata = scratchImage.GetMetadata();
        stagingBuffers[4 + i] = UploadTexture(m_MainTextures[i], scratchImage.GetPixels(), metadata.width, metadata.height, 4);
    }
    
    EndCommandList();

    ExecuteCommandBuffer();

    // Allocate descriptor set
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

    std::array<VkWriteDescriptorSet, 6> descriptorWrites{};
    VkDescriptorBufferInfo cameraBufferInfo = CreateDescriptorBufferInfo(m_CameraDataBuffer, CameraData::GetAlignedByteSizes());
    UpdateBufferDescriptor(descriptorWrites[0], m_DescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &cameraBufferInfo, GetBindingSlot(ERegisterType::ConstantBuffer, 0)); // _CameraData 
    
    VkDescriptorBufferInfo lightBufferInfo = CreateDescriptorBufferInfo(m_LightDataBuffer, DirectionalLightData::GetAlignedByteSizes());
    UpdateBufferDescriptor(descriptorWrites[1], m_DescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &lightBufferInfo, GetBindingSlot(ERegisterType::ConstantBuffer, 1)); // _LightData
    
    VkDescriptorBufferInfo instanceBufferInfo = CreateDescriptorBufferInfo(m_InstanceBuffer, instanceBufferBytesSize);
    UpdateBufferDescriptor(descriptorWrites[2], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &instanceBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 0)); // _InstancesBuffer

    std::array<VkDescriptorImageInfo, s_TexturesCount> imageDescriptorInfos;
    std::array<VkDescriptorImageInfo, s_TexturesCount> samplerDescriptorInfos;
    
    for(uint32_t i = 0; i < s_TexturesCount; ++i)
    {
        imageDescriptorInfos[i] = CreateDescriptorImageInfo(m_MainTextureViews[i], VK_NULL_HANDLE);
        samplerDescriptorInfos[i] = CreateDescriptorImageInfo(VK_NULL_HANDLE, m_MainTextureSamplers[i]);
    }

    UpdateImageDescriptor(descriptorWrites[3]
        , m_DescriptorSet
        , VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        , imageDescriptorInfos.data()
        , (uint32_t)imageDescriptorInfos.size()
        , GetBindingSlot(ERegisterType::ShaderResource, 1)); // _MainTex
    
    VkDescriptorBufferInfo materialsBufferInfo = CreateDescriptorBufferInfo(m_MaterialsBuffer, materialsBufferBytesSize);
    UpdateBufferDescriptor(descriptorWrites[4]
        , m_DescriptorSet
        , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        , &materialsBufferInfo
        , GetBindingSlot(ERegisterType::ShaderResource, 6)); // _MaterialsBuffer

    UpdateImageDescriptor(descriptorWrites[5]
        , m_DescriptorSet
        , VK_DESCRIPTOR_TYPE_SAMPLER
        , samplerDescriptorInfos.data()
        , (uint32_t)samplerDescriptorInfos.size()
        , GetBindingSlot(ERegisterType::Sampler, 0)); // _MainTexSampler
    

    vkUpdateDescriptorSets(m_DeviceHandle, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    
    return true;
}

void GraphicsPipelineVk::DestroyResources()
{
    if(m_DescriptorSet != VK_NULL_HANDLE)
    {
        vkFreeDescriptorSets(m_DeviceHandle, m_DescriptorPoolHandle, 1, &m_DescriptorSet);
        m_DescriptorSet = VK_NULL_HANDLE;
    }
    
    for(uint32_t i = 0; i < s_TexturesCount; ++i)
    {
        if(m_MainTextureSamplers[i] != VK_NULL_HANDLE)
            vkDestroySampler(m_DeviceHandle, m_MainTextureSamplers[i], nullptr);
        if(m_MainTextureViews[i] != VK_NULL_HANDLE)
            vkDestroyImageView(m_DeviceHandle, m_MainTextureViews[i], nullptr);
        if(m_MainTextureMemories[i] != VK_NULL_HANDLE)
            vkFreeMemory(m_DeviceHandle, m_MainTextureMemories[i], nullptr);
        if(m_MainTextures[i] != VK_NULL_HANDLE)
            vkDestroyImage(m_DeviceHandle, m_MainTextures[i], nullptr);
    }
    if(m_MaterialsBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_MaterialsBufferMemory, nullptr);
        m_MaterialsBufferMemory = VK_NULL_HANDLE;
    }
    if(m_MaterialsBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_MaterialsBuffer, nullptr);
        m_MaterialsBuffer = VK_NULL_HANDLE;
    }
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
    if(m_IndicesBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_IndicesBufferMemory, nullptr);
        m_IndicesBufferMemory = VK_NULL_HANDLE;
    }
    if(m_IndicesBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_IndicesBuffer, nullptr);
        m_IndicesBuffer = VK_NULL_HANDLE;
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
    if(m_LightDataMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_LightDataMemory, nullptr);
        m_LightDataMemory = VK_NULL_HANDLE;
    }
    if(m_LightDataBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_LightDataBuffer, nullptr);
        m_LightDataBuffer = VK_NULL_HANDLE;
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