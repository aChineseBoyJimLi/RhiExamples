#include "MeshPipelineVk.h"
#include <array>

bool MeshPipelineVk::CreateDescriptorLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 0), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _CameraData
	bindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 1), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _ViewFrustum
	bindings.push_back({GetBindingSlot(ERegisterType::ConstantBuffer, 2), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _MeshInfo
	bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 0), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _Vertices
	bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 1), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _TexCoords
	bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 2), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _Meshlets
	bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 3), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _PackedPrimitiveIndices
	bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 4), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _UniqueVertexIndices
	bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 5), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _MeshletCullData
	bindings.push_back({GetBindingSlot(ERegisterType::ShaderResource, 6), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT, nullptr}); // _InstanceData

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

void MeshPipelineVk::DestroyDescriptorLayout()
{
    if(m_DescriptorLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_DeviceHandle, m_DescriptorLayout, nullptr);
        m_DescriptorLayout = VK_NULL_HANDLE;
    }
}

bool MeshPipelineVk::CreateShader()
{
    m_ASBlob = AssetsManager::LoadShaderImmediately("VisibleCulling.as.spv");
    if(!m_ASBlob || m_ASBlob->IsEmpty())
    {
        Log::Error("Failed to load task shader");
        return false;
    }
    m_MSBlob = AssetsManager::LoadShaderImmediately("MeshletViewer.ms.spv");
    if(!m_MSBlob || m_MSBlob->IsEmpty())
    {
        Log::Error("Failed to load mesh shader");
        return false;
    }
	m_PSBlob = AssetsManager::LoadShaderImmediately("SolidColor.ps.spv");
	if(!m_PSBlob || m_PSBlob->IsEmpty())
	{
		Log::Error("Failed to load pixel shader");
		return false;
	}

    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = m_ASBlob->GetSize();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(m_ASBlob->GetData());
    VkResult result = vkCreateShaderModule(m_DeviceHandle, &shaderInfo, nullptr, &m_ASModule);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create task shader");
		return false;
	}

	shaderInfo.codeSize = m_MSBlob->GetSize();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(m_MSBlob->GetData());
	result = vkCreateShaderModule(m_DeviceHandle, &shaderInfo, nullptr, &m_MSModule);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create mesh shader");
		return false;
	}

    shaderInfo.codeSize = m_PSBlob->GetSize();
    shaderInfo.pCode = reinterpret_cast<const uint32_t*>(m_PSBlob->GetData());
    result = vkCreateShaderModule(m_DeviceHandle, &shaderInfo, nullptr, &m_PSModule);
	if(result != VK_SUCCESS)
	{
		Log::Error("Failed to create pixel shader");
		return false;
	}
	
    return true;
}

void MeshPipelineVk::DestroyShader()
{
	if(m_ASModule != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(m_DeviceHandle, m_ASModule, nullptr);
		m_ASModule = VK_NULL_HANDLE;
	}
	
    if(m_MSModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_DeviceHandle, m_MSModule, nullptr);
        m_MSModule = VK_NULL_HANDLE;
    }

    if(m_PSModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_DeviceHandle, m_PSModule, nullptr);
        m_PSModule = VK_NULL_HANDLE;
    }
}

bool MeshPipelineVk::CreateRenderPass()
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

void MeshPipelineVk::DestroyRenderPass()
{
    if(m_RenderPassHandle != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_DeviceHandle, m_RenderPassHandle, nullptr);
        m_RenderPassHandle = VK_NULL_HANDLE;
    }
}

bool MeshPipelineVk::CreatePipelineState()
{
	// Create pipeline state for graphics pass --------------------------
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
			{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_TASK_BIT_EXT, m_ASModule, "main", nullptr},
		{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_MESH_BIT_EXT, m_MSModule, "main", nullptr},
		{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, m_PSModule, "main", nullptr}
	};
	
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
	pipelineInfo.pVertexInputState = nullptr;
	pipelineInfo.pInputAssemblyState = nullptr;
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
	
	return true;
}

void MeshPipelineVk::DestroyPipelineState()
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