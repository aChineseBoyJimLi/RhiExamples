#include "IndirectDrawVk.h"
#include <array>

bool IndirectDrawVk::CreateDescriptorLayout()
{
	// Create descriptor set layout for graphics pass ---------------------
	// Descriptor Set 0 Layout
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}); // _CameraData
    bindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 1), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}); // _LightData
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 0), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}); // _InstanceData
    bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 1), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}); // _MaterialData
	bindings.push_back({GetBindingSlot(ERegisterType::Sampler, 0), VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}); // _MainTex_Sampler

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkResult result = vkCreateDescriptorSetLayout(m_DeviceHandle, &layoutInfo, nullptr, &m_DescriptorLayoutSpace0);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create descriptor set 0 layout");
        return false;
    }

	// Descriptor Set 1 Layout
	std::vector<VkDescriptorSetLayoutBinding> bindings2;
	bindings2.push_back({GetBindingSlot(ERegisterType::ShaderResource, 0), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, s_TexturesCount, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}); // _MainTex
	
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo{};
	bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	bindingFlagsInfo.bindingCount = static_cast<uint32_t>(bindings2.size());
	VkDescriptorBindingFlagsEXT bindingFlagsData[1] = {
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT // for _MainTex
	};
	bindingFlagsInfo.pBindingFlags = bindingFlagsData;

	VkDescriptorSetLayoutCreateInfo layoutInfo1{};
	layoutInfo1.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo1.bindingCount = static_cast<uint32_t>(bindings2.size());
	layoutInfo1.pBindings = bindings2.data();
	layoutInfo1.pNext = &bindingFlagsInfo;

	result = vkCreateDescriptorSetLayout(m_DeviceHandle, &layoutInfo1, nullptr, &m_DescriptorLayoutSpace1);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create descriptor set 1 layout");
		return false;
	}
	
	// Create descriptor layout for culling pass ------------------------
	std::vector<VkDescriptorSetLayoutBinding> cullingPassBindings;
	cullingPassBindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}); // _CameraData
	cullingPassBindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 1), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}); // _ViewFrustum
	cullingPassBindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 2), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}); // _AABB
	cullingPassBindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 0), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}); // _InstancesData
	cullingPassBindings.push_back({GetBindingSlot(ERegisterType::UnorderedAccess, 0), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}); // _IndirectCommands
	
	layoutInfo.bindingCount = static_cast<uint32_t>(cullingPassBindings.size());
	layoutInfo.pBindings = cullingPassBindings.data();
	

	result = vkCreateDescriptorSetLayout(m_DeviceHandle, &layoutInfo, nullptr, &m_CullingPassDescriptorSetLayout);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create descriptor set layout  for culling pass");
		return false;
	}
	
    return true;
}

void IndirectDrawVk::DestroyDescriptorLayout()
{
	if(m_CullingPassDescriptorSetLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_DeviceHandle, m_CullingPassDescriptorSetLayout, nullptr);
		m_CullingPassDescriptorSetLayout = VK_NULL_HANDLE;
	}
	if(m_DescriptorLayoutSpace1 != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_DeviceHandle, m_DescriptorLayoutSpace1, nullptr);
		m_DescriptorLayoutSpace1 = VK_NULL_HANDLE;
	}
	
	if(m_DescriptorLayoutSpace0 != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_DeviceHandle, m_DescriptorLayoutSpace0, nullptr);
		m_DescriptorLayoutSpace0 = VK_NULL_HANDLE;
	}
}

bool IndirectDrawVk::CreateShader()
{
    m_VertexShaderBlob = AssetsManager::LoadShaderImmediately("Graphics.vs.spv");
    if(!m_VertexShaderBlob || m_VertexShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load vertex shader");
        return false;
    }
    m_PixelShaderBlob = AssetsManager::LoadShaderImmediately("Graphics.ps.spv");
    if(!m_PixelShaderBlob || m_PixelShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load pixel shader");
        return false;
    }
	m_ComputeShaderBlob = AssetsManager::LoadShaderImmediately("VisibleCullingVk.cs.spv");
	if(!m_ComputeShaderBlob || m_ComputeShaderBlob->IsEmpty())
	{
		Log::Error("Failed to load compute shader");
		return false;
	}

    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = m_VertexShaderBlob->GetSize();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(m_VertexShaderBlob->GetData());
    VkResult result = vkCreateShaderModule(m_DeviceHandle, &shaderInfo, nullptr, &m_VertexShaderModule);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create vertex shader");
		return false;
	}

    shaderInfo.codeSize = m_PixelShaderBlob->GetSize();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(m_PixelShaderBlob->GetData());
    result = vkCreateShaderModule(m_DeviceHandle, &shaderInfo, nullptr, &m_PixelShaderModule);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create pixel shader");
		return false;
	}

	shaderInfo.codeSize = m_ComputeShaderBlob->GetSize();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(m_ComputeShaderBlob->GetData());
	result = vkCreateShaderModule(m_DeviceHandle, &shaderInfo, nullptr, &m_ComputeShaderModule);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create compute shader");
		return false;
	}
	
    return true;
}

void IndirectDrawVk::DestroyShader()
{
	if(m_ComputeShaderModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(m_DeviceHandle, m_ComputeShaderModule, nullptr);
		m_ComputeShaderModule = VK_NULL_HANDLE;
	}
	
    if(m_VertexShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_DeviceHandle, m_VertexShaderModule, nullptr);
        m_VertexShaderModule = VK_NULL_HANDLE;
    }

    if(m_PixelShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_DeviceHandle, m_PixelShaderModule, nullptr);
        m_PixelShaderModule = VK_NULL_HANDLE;
    }
}

bool IndirectDrawVk::CreateRenderPass()
{
    VkAttachmentDescription colorAttachment{};
	colorAttachment.format = s_BackBufferFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = s_DepthStencilFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment , depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(m_DeviceHandle, &renderPassInfo, nullptr, &m_RenderPassHandle);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create render pass");
		return false;
	}
	
    return true;
}

void IndirectDrawVk::DestroyRenderPass()
{
    if(m_RenderPassHandle != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_DeviceHandle, m_RenderPassHandle, nullptr);
        m_RenderPassHandle = VK_NULL_HANDLE;
    }
}

bool IndirectDrawVk::CreatePipelineState()
{
	// Create pipeline state for graphics pass --------------------------
	VkDescriptorSetLayout layouts[2] = {m_DescriptorLayoutSpace0, m_DescriptorLayoutSpace1};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = layouts;
	VkResult result = vkCreatePipelineLayout(m_DeviceHandle, &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create pipeline layout");
		return false;
	}

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages
	{
		{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, m_VertexShaderModule, "main", nullptr},
		{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, m_PixelShaderModule, "main", nullptr}
	};
	
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions
	{
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
		{1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12},
		{2, 0, VK_FORMAT_R32G32_SFLOAT, 24}
	};

	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(VertexData);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicStates
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = (uint32_t)shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.renderPass = m_RenderPassHandle;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	result = vkCreateGraphicsPipelines(m_DeviceHandle, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineState);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create pipeline");
		return false;
	}

	// Create pipeline state for culling pass ---------------------------
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &m_CullingPassDescriptorSetLayout;
	result = vkCreatePipelineLayout(m_DeviceHandle, &pipelineLayoutInfo, nullptr, &m_CullingPassPipelineLayout);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create culling pass pipeline layout");
		return false;
	}

	VkComputePipelineCreateInfo computePipelineInfo{};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computePipelineInfo.stage.module = m_ComputeShaderModule;
	computePipelineInfo.stage.pName = "main";
	computePipelineInfo.stage.pSpecializationInfo = nullptr;
	computePipelineInfo.stage.flags = 0;
	computePipelineInfo.stage.pNext = nullptr;
	computePipelineInfo.layout = m_CullingPassPipelineLayout;
	computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	computePipelineInfo.basePipelineIndex = -1;
	result = vkCreateComputePipelines(m_DeviceHandle, VK_NULL_HANDLE, 1,  &computePipelineInfo, nullptr, &m_CullingPassPipelineState);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create culling pass pipeline layout");
		return false;
	}
	
	return true;
}

void IndirectDrawVk::DestroyPipelineState()
{
	if(m_CullingPassPipelineState != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_DeviceHandle, m_CullingPassPipelineState, nullptr);
		m_CullingPassPipelineState = VK_NULL_HANDLE;
	}
	
	if(m_CullingPassPipelineLayout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(m_DeviceHandle, m_CullingPassPipelineLayout, nullptr);
		m_CullingPassPipelineLayout = VK_NULL_HANDLE;
	}
	
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