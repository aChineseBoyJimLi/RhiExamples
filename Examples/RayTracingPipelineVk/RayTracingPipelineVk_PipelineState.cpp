#include "RayTracingPipelineVk.h"
#include <array>

bool RayTracingPipelineVk::CreateDescriptorLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}); // _CameraData
    bindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 1), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}); // _LightData
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 0), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}); // _InstanceData
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 1), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 5, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}); // _MainTex
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 6), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}); // _MaterialData
    bindings.push_back({GetBindingSlot(ERegisterType::Sampler, 0), VK_DESCRIPTOR_TYPE_SAMPLER, 5, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}); // _MainTex_Sampler

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
    
	
    return true;
}

void RayTracingPipelineVk::DestroyShader()
{
    
}

bool RayTracingPipelineVk::CreatePipelineState()
{
	
	
	return true;
}

void RayTracingPipelineVk::DestroyPipelineState()
{
	
}