#include "RayTracingPipelineVk.h"
#include <array>

bool RayTracingPipelineVk::CreateDescriptorLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr}); // _CameraData
    bindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 1), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr}); // _LightData
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 0), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_ALL, nullptr}); // _AccelStructure
    bindings.push_back({GetBindingSlot(ERegisterType::UnorderedAccess, 0), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_ALL, nullptr}); // _Output
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 1), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr}); // _Indices
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 2), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr}); // _Texcoords
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 3), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr}); // _Normals
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 4), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr}); // _InstanceData
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 5), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr}); // _MaterialData
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(m_DeviceHandle, &layoutInfo, nullptr, &m_DescriptorLayout);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create descriptor set layout");
        return false;
    }
    
    return true;
}

void RayTracingPipelineVk::DestroyDescriptorLayout()
{
    if(m_DescriptorLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_DeviceHandle, m_DescriptorLayout, nullptr);
        m_DescriptorLayout = VK_NULL_HANDLE;
    }
}

bool RayTracingPipelineVk::CreateShader()
{
    m_RayGenShaderBlob = AssetsManager::LoadShaderImmediately("RayTracing.rgen.spv");
    if(!m_RayGenShaderBlob || m_RayGenShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load the ray generation shader");
        return false;
    }
    m_MissShadersBlob = AssetsManager::LoadShaderImmediately("RayTracing.rmis.spv");
    if(!m_MissShadersBlob || m_MissShadersBlob->IsEmpty())
    {
        Log::Error("Failed to load the miss shader");
        return false;
    }
    m_HitGroupShadersBlob = AssetsManager::LoadShaderImmediately("RayTracing.rhit.spv");
    if(!m_HitGroupShadersBlob || m_HitGroupShadersBlob->IsEmpty())
    {
        Log::Error("Failed to load the hit group shader");
        return false;
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = m_RayGenShaderBlob->GetSize();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(m_RayGenShaderBlob->GetData());

    VkResult result = vkCreateShaderModule(m_DeviceHandle, &createInfo, nullptr, &m_RayGenShaderModule);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create ray gen shader");
        return false;
    }

    createInfo.codeSize = m_MissShadersBlob->GetSize();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(m_MissShadersBlob->GetData());

    result = vkCreateShaderModule(m_DeviceHandle, &createInfo, nullptr, &m_MissShaderModule);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create miss shader");
        return false;
    }

    createInfo.codeSize = m_HitGroupShadersBlob->GetSize();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(m_HitGroupShadersBlob->GetData());

    result = vkCreateShaderModule(m_DeviceHandle, &createInfo, nullptr, &m_ClosestHitShaderModule);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create hit group shader");
        return false;
    }
    
    return true;
}

void RayTracingPipelineVk::DestroyShader()
{
    if(m_RayGenShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_DeviceHandle, m_RayGenShaderModule, nullptr);
        m_RayGenShaderModule = VK_NULL_HANDLE;
    }
    if(m_MissShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_DeviceHandle, m_MissShaderModule, nullptr);
        m_MissShaderModule = VK_NULL_HANDLE;
    }
    if(m_ClosestHitShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_DeviceHandle, m_ClosestHitShaderModule, nullptr);
        m_ClosestHitShaderModule = VK_NULL_HANDLE;
    }
}

bool RayTracingPipelineVk::CreatePipelineState()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorLayout;
    VkResult result = vkCreatePipelineLayout(m_DeviceHandle, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create pipeline layout");
        return false;
    }
    
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages
    {
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_RAYGEN_BIT_KHR, m_RayGenShaderModule, "RayGen", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_MISS_BIT_KHR, m_MissShaderModule, "RayMiss", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_MISS_BIT_KHR, m_MissShaderModule, "ShadowMiss", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, m_ClosestHitShaderModule, "ClosestHitTriangle", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, m_ClosestHitShaderModule, "ClosestHitProceduralPrim", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_INTERSECTION_BIT_KHR, m_ClosestHitShaderModule, "ProceduralPlanePrim", nullptr}
    };

    m_ShaderGroups.clear();

    // RayGen
    m_ShaderGroups.push_back({
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        nullptr,
        VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        0,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR
    });

    // RayMiss
    m_ShaderGroups.push_back({
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        nullptr,
        VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        1,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR
    });

    // ShadowMiss
    m_ShaderGroups.push_back({
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        nullptr,
        VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        2,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR
    });

    // ClosestHitTriangle
    m_ShaderGroups.push_back({
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        nullptr,
        VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
        VK_SHADER_UNUSED_KHR,
        3,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR
    });

    // ClosestHitProceduralPrim + ProceduralPlanePrim
    m_ShaderGroups.push_back({
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        nullptr,
        VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
        VK_SHADER_UNUSED_KHR,
        4,
        VK_SHADER_UNUSED_KHR,
        5
    });

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(m_ShaderGroups.size());
    pipelineInfo.pGroups = m_ShaderGroups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = s_MaxRayRecursionDepth;
    pipelineInfo.layout = m_PipelineLayout;
    
    result = vkCreateRayTracingPipelinesKHR(m_DeviceHandle, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineState);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create ray tracing pipeline");
        return false;
    }
	
	return true;
}

void RayTracingPipelineVk::DestroyPipelineState()
{
	if(m_PipelineState != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_DeviceHandle, m_PipelineState, nullptr);
        m_PipelineState = VK_NULL_HANDLE;
    }

    if(m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_DeviceHandle, m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }
}

static size_t AlignedSize(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

bool RayTracingPipelineVk::CreateShaderTable()
{
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
    rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProperties2{};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &rayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(m_GpuHandle, &deviceProperties2);

    const uint32_t handleSizeAligned = (uint32_t)AlignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment); // shader group handle size
     
    const uint32_t shaderTableItemAlignmentSize = (uint32_t)AlignedSize(handleSizeAligned, rayTracingPipelineProperties.shaderGroupBaseAlignment); // shader table items size
    const uint32_t groupCount = (uint32_t)m_ShaderGroups.size();
    const uint32_t shaderTableSize = groupCount * shaderTableItemAlignmentSize;
    const uint32_t shaderGroupHandlesSize = groupCount * handleSizeAligned;
    std::vector<uint8_t> shaderHandleStorage(shaderGroupHandlesSize);
    
    VkResult result = vkGetRayTracingShaderGroupHandlesKHR(m_DeviceHandle, m_PipelineState, 0, groupCount, shaderGroupHandlesSize, shaderHandleStorage.data());
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to get ray tracing shader group handles");
        return false;
    }

    if(!CreateBuffer(shaderTableSize
        , VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        , m_ShaderTableBuffer
        , m_ShaderTableBufferMemory))
    {
        Log::Error("Failed to create shader table buffer");
        return false;
    }

    const uint8_t* shaderHandleStorageData = shaderHandleStorage.data();
    for(uint32_t i = 0; i < groupCount; ++i)
    {
        WriteBufferData(m_DeviceHandle
            , m_ShaderTableBufferMemory
            , shaderHandleStorageData + i * handleSizeAligned
            , handleSizeAligned
            , i * shaderTableItemAlignmentSize);
    }

    VkDeviceAddress shaderTableAddress =  GetBufferDeviceAddress(m_ShaderTableBuffer);
    constexpr uint32_t ragGenShaderCount = 1, missShaderCount = 2, hitGroupCount = 2;
    uint32_t shaderTableIndex = 0;
    
    m_RaygenShaderSbtEntry.deviceAddress = shaderTableAddress;
    m_RaygenShaderSbtEntry.stride = shaderTableItemAlignmentSize;
    m_RaygenShaderSbtEntry.size = shaderTableItemAlignmentSize;
    shaderTableIndex += ragGenShaderCount;

    m_MissShaderSbtEntry.deviceAddress = shaderTableAddress + shaderTableIndex * shaderTableItemAlignmentSize;
    m_MissShaderSbtEntry.stride = shaderTableItemAlignmentSize;
    m_MissShaderSbtEntry.size = missShaderCount * shaderTableItemAlignmentSize;
    shaderTableIndex += missShaderCount;

    m_HitShaderSbtEntry.deviceAddress = shaderTableAddress + shaderTableIndex * shaderTableItemAlignmentSize;
    m_HitShaderSbtEntry.stride = shaderTableItemAlignmentSize;
    m_HitShaderSbtEntry.size = hitGroupCount * shaderTableItemAlignmentSize;
    
    return true;
}

void RayTracingPipelineVk::DestroyShaderTable()
{
    if(m_ShaderTableBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_ShaderTableBufferMemory, nullptr);
        m_ShaderTableBufferMemory = VK_NULL_HANDLE;
    }
    if(m_ShaderTableBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_ShaderTableBuffer, nullptr);
        m_ShaderTableBuffer = VK_NULL_HANDLE;
    }
}