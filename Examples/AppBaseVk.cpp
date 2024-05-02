#include "AppBaseVk.h"
#include <array>

void WriteBufferData(VkDevice inDevice, VkDeviceMemory inBuffer, const void* inData, size_t inSize, size_t inOffset)
{
    if(inDevice == VK_NULL_HANDLE || inBuffer == VK_NULL_HANDLE)
        return;
    
    void* mappedData;
    vkMapMemory(inDevice, inBuffer, inOffset, inSize, 0, &mappedData);
    memcpy(mappedData, inData, inSize);
    vkUnmapMemory(inDevice, inBuffer);
}

uint32_t GetBindingSlot(ERegisterType registerType, uint32_t inRegisterSlot)
{
    switch (registerType)
    {
        case ERegisterType::ConstantBuffer:
            return inRegisterSlot + SPIRV_CBV_BINDING_OFFSET;
        case ERegisterType::ShaderResource:
            return inRegisterSlot + SPIRV_SRV_BINDING_OFFSET;
        case ERegisterType::UnorderedAccess:
            return inRegisterSlot + SPIRV_UAV_BINDING_OFFSET;
        case ERegisterType::Sampler:
            return inRegisterSlot + SPIRV_SAMPLER_BINDING_OFFSET;
        default:
            return inRegisterSlot;
    }
}

VkDescriptorBufferInfo CreateDescriptorBufferInfo(VkBuffer inBuffer, size_t inSize)
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = inBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = inSize;
    return bufferInfo;
}

VkDescriptorImageInfo CreateDescriptorImageInfo(VkImageView inImageView, VkSampler inSampler)
{
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = inImageView;
    imageInfo.sampler = inSampler;
    return imageInfo;
}

void UpdateBufferDescriptor(VkWriteDescriptorSet& outDescriptor, VkDescriptorSet inSet, VkDescriptorType inType, VkDescriptorBufferInfo* inBufferInfo, uint32_t inBinding)
{
    outDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    outDescriptor.dstSet = inSet;
    outDescriptor.dstBinding = inBinding;
    outDescriptor.descriptorType = inType;
    outDescriptor.descriptorCount = 1;
    outDescriptor.pBufferInfo = inBufferInfo;
}

void UpdateImageDescriptor(VkWriteDescriptorSet& outDescriptor, VkDescriptorSet inSet, VkDescriptorType inType, VkDescriptorImageInfo* inImageInfo, uint32_t inDescriptorCount, uint32_t inBinding)
{
    outDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    outDescriptor.dstSet = inSet;
    outDescriptor.dstBinding = inBinding;
    outDescriptor.descriptorType = inType;
    outDescriptor.descriptorCount = inDescriptorCount;
    outDescriptor.pImageInfo = inImageInfo;
}

bool AppBaseVk::CreateCommandList()
{
    VkCommandPoolCreateInfo cmdPoolCreateInfo{};
    cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolCreateInfo.queueFamilyIndex = m_QueueIndex;

    VkResult result = vkCreateCommandPool(m_DeviceHandle, &cmdPoolCreateInfo, nullptr, &m_CmdPoolHandle);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create command pool");
        return false;
    }

    VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.commandPool = m_CmdPoolHandle;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = 1;
    
    result = vkAllocateCommandBuffers(m_DeviceHandle, &cmdBufferAllocInfo, &m_CmdBufferHandle);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to allocate command buffer");
        return false;
    }
    
    return true;
}

void AppBaseVk::DestroyCommandList()
{
    if(m_CmdBufferHandle != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(m_DeviceHandle, m_CmdPoolHandle, 1, &m_CmdBufferHandle);
        m_CmdBufferHandle = VK_NULL_HANDLE;
    }

    if(m_CmdPoolHandle != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_DeviceHandle, m_CmdPoolHandle, nullptr);
        m_CmdPoolHandle = VK_NULL_HANDLE;
    }
}

void AppBaseVk::BeginCommandList()
{
    if(m_CmdBufferIsClosed)
    {
        vkResetCommandPool(m_DeviceHandle, m_CmdPoolHandle, 0);
        vkResetCommandBuffer(m_CmdBufferHandle, 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(m_CmdBufferHandle, &beginInfo);
        m_CmdBufferIsClosed = false;
    }
}

void AppBaseVk::EndCommandList()
{
    if (!m_CmdBufferIsClosed)
    {
        vkEndCommandBuffer(m_CmdBufferHandle);
        m_CmdBufferIsClosed = true;
    }
}

int AppBaseVk::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    for (uint32_t i = 0; i < m_GpuMemoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (m_GpuMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return -1;
}

bool AppBaseVk::CreateDescriptorSetPool()
{
    std::array<VkDescriptorPoolSize, 9> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    poolSizes[0].descriptorCount = 512;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    poolSizes[1].descriptorCount = 1024;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[2].descriptorCount = 1024;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    poolSizes[3].descriptorCount = 1024;
    poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[4].descriptorCount = 1024;
    poolSizes[5].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[5].descriptorCount = 1024;
    poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[6].descriptorCount = 1024;
    poolSizes[7].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[7].descriptorCount = 1024;
    poolSizes[8].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    poolSizes[8].descriptorCount = 1024;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 100; 
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkResult result = vkCreateDescriptorPool(m_DeviceHandle, &poolInfo, nullptr, &m_DescriptorPoolHandle);

    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create descriptor pool");
        return false;
    }
    
    return true;
}

void AppBaseVk::DestroyDescriptorSetPool()
{
    if(m_DescriptorPoolHandle != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(m_DeviceHandle, m_DescriptorPoolHandle, nullptr);
        m_DescriptorPoolHandle = VK_NULL_HANDLE;
    }
}

bool AppBaseVk::CreateBuffer(size_t inSize
    , VkBufferUsageFlags inUsage
    , VkMemoryPropertyFlags inProperties
    , VkBuffer& outBuffer
    , VkDeviceMemory& outBufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = inSize;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = inUsage;
    VkResult result = vkCreateBuffer(m_DeviceHandle, &bufferInfo, nullptr, &outBuffer);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create buffer");
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_DeviceHandle, outBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;

    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (inUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        allocInfo.pNext = &allocFlagsInfo;
    }
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, inProperties);

    result = vkAllocateMemory(m_DeviceHandle, &allocInfo, nullptr, &outBufferMemory);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to allocate memory for buffer");
        return false;
    }
    
    result = vkBindBufferMemory(m_DeviceHandle, outBuffer, outBufferMemory, 0);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to bind buffer memory");
        return false;
    }
    
    return true;
}

bool AppBaseVk::CreateTexture(size_t inWidth
            , size_t inHeight
            , VkFormat inFormat
            , VkImageUsageFlags inUsage
            , VkMemoryPropertyFlags inProperties
            , VkImageLayout inLayout
            , VkImage& outImage
            , VkDeviceMemory& outImageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.extent.width = (uint32_t)inWidth;
    imageInfo.extent.height = (uint32_t)inHeight;
    imageInfo.extent.depth = 1;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.format = inFormat;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.usage = inUsage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = inLayout;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;

    VkResult result = vkCreateImage(m_DeviceHandle, &imageInfo, nullptr, &outImage);

    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create texture 2d");
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_DeviceHandle, outImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, inProperties);

    result = vkAllocateMemory(m_DeviceHandle, &allocInfo, nullptr, &outImageMemory);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to allocate memory for texture 2d");
        return false;
    }

    result = vkBindImageMemory(m_DeviceHandle, outImage, outImageMemory, 0);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to bind image memory");
        return false;
    }
    
    return true;
}

bool AppBaseVk::CreateImageView(VkImage inImage
            , VkFormat inFormat
            , VkImageAspectFlags inAspect
            , VkImageView& outImageView)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = inImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = inFormat;
    viewInfo.subresourceRange.aspectMask = inAspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkResult result = vkCreateImageView(m_DeviceHandle, &viewInfo, nullptr, &outImageView);
    if(result != VK_SUCCESS)
    {
        return false;
    }
    return true;
}

std::shared_ptr<StagingBuffer> AppBaseVk::CreateStagingBuffer(size_t inSize)
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    if(!CreateBuffer(inSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer, memory))
    {
        Log::Error("Failed to create staging buffer");
        return nullptr;
    }
    return std::make_shared<StagingBuffer>(m_DeviceHandle, buffer, memory);
}

VkImageMemoryBarrier AppBaseVk::ImageMemoryBarrier(VkImage inImage
        , VkImageLayout inOldLayout
        , VkImageLayout inNewLayout
        , VkAccessFlagBits inSrcAccessMask
        , VkAccessFlagBits inDstAccessMask
        , VkImageAspectFlags inAspectMask)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = inOldLayout;
    barrier.newLayout = inNewLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = inImage;
    barrier.subresourceRange.aspectMask = inAspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = inSrcAccessMask;
    barrier.dstAccessMask = inDstAccessMask;
    return barrier;
}

bool AppBaseVk::CreateSampler(VkSampler& outSampler)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = m_GpuProperties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	
    if(vkCreateSampler(m_DeviceHandle, &samplerInfo, nullptr, &outSampler) != VK_SUCCESS)
    {
        Log::Error("Failed to create sampler");
        return false;
    }

    return true;
}

std::shared_ptr<StagingBuffer> AppBaseVk::UploadBuffer(VkBuffer dstBuffer, const void* srcData, size_t size, size_t dstOffset)
{
    std::shared_ptr<StagingBuffer> stagingBuffer = CreateStagingBuffer(size);
    if(stagingBuffer == nullptr)
    {
        return nullptr;
    }
    
    WriteBufferData(m_DeviceHandle, stagingBuffer->m_Memory, srcData, size, 0);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = dstOffset;
    vkCmdCopyBuffer(m_CmdBufferHandle, stagingBuffer->m_Buffer, dstBuffer, 1, &copyRegion);

    return stagingBuffer;
}

std::shared_ptr<StagingBuffer> AppBaseVk::UploadTexture(VkImage dstTexture, const void* srcData, size_t width, size_t height, uint32_t channels)
{
    size_t size = width * height * channels;
    std::shared_ptr<StagingBuffer> stagingBuffer = CreateStagingBuffer(size);
    if(stagingBuffer == nullptr)
    {
        return nullptr;
    }

    WriteBufferData(m_DeviceHandle, stagingBuffer->m_Memory, srcData, size, 0);

    VkImageMemoryBarrier barrier = ImageMemoryBarrier(dstTexture
        , VK_IMAGE_LAYOUT_UNDEFINED
        , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        , VK_ACCESS_NONE
        , VK_ACCESS_TRANSFER_WRITE_BIT
        , VK_IMAGE_ASPECT_COLOR_BIT);
    
    vkCmdPipelineBarrier(
        m_CmdBufferHandle,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
    
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

    vkCmdCopyBufferToImage(m_CmdBufferHandle, stagingBuffer->m_Buffer, dstTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


    barrier = ImageMemoryBarrier(dstTexture
        , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        , VK_ACCESS_TRANSFER_WRITE_BIT
        , VK_ACCESS_SHADER_READ_BIT
        , VK_IMAGE_ASPECT_COLOR_BIT);
    
    vkCmdPipelineBarrier(
        m_CmdBufferHandle,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    return stagingBuffer;
}

bool AppBaseVk::CreateSwapChain()
{
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_GpuHandle, m_SurfaceHandle, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> availableSurfaceformats;
    if (formatCount > 0) {
        availableSurfaceformats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_GpuHandle, m_SurfaceHandle, &formatCount, availableSurfaceformats.data());
    }

    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    
    for (const auto& AvailableFormat : availableSurfaceformats) {
        if (AvailableFormat.format == s_BackBufferFormat) {
            colorSpace = AvailableFormat.colorSpace;
            break;
        }
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_GpuHandle, m_SurfaceHandle, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> availablePresentModes;
    if (presentModeCount > 0) {
        availablePresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_GpuHandle, m_SurfaceHandle, &presentModeCount, availablePresentModes.data());
    }
    VkPresentModeKHR swapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapChainPresentMode = availablePresentMode;
            break;
        }
    }
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_GpuHandle, m_SurfaceHandle, &m_Capabilities);
    
    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = m_SurfaceHandle;
    swapChainCreateInfo.minImageCount = s_BackBufferCount;
    swapChainCreateInfo.imageFormat = s_BackBufferFormat;
    swapChainCreateInfo.imageColorSpace = colorSpace;
    swapChainCreateInfo.imageExtent = m_Capabilities.currentExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = m_QueueIndex;
    swapChainCreateInfo.preTransform = m_Capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = swapChainPresentMode;
    swapChainCreateInfo.clipped = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(m_DeviceHandle, &swapChainCreateInfo, nullptr, &m_SwapChainHandle);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create swap chain");
        return false;
    }
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_DeviceHandle, m_SwapChainHandle, &imageCount, nullptr);
    m_BackBuffers.resize(imageCount);
    m_BackBufferViews.resize(imageCount);
    vkGetSwapchainImagesKHR(m_DeviceHandle, m_SwapChainHandle, &imageCount, m_BackBuffers.data());
    for(uint32_t i = 0; i < imageCount; ++i)
    {
        CreateImageView(m_BackBuffers[i], s_BackBufferFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_BackBufferViews[i]);
    }

    vkAcquireNextImageKHR(m_DeviceHandle, m_SwapChainHandle, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &m_CurrentIndex);
    
    return true;
}

void AppBaseVk::DestroySwapChain()
{
    for(auto view : m_BackBufferViews)
    {
        if(view != VK_NULL_HANDLE)
            vkDestroyImageView(m_DeviceHandle, view, nullptr);
    }
    m_BackBufferViews.clear();
    
    if(m_SwapChainHandle != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_DeviceHandle, m_SwapChainHandle, nullptr);
        m_SwapChainHandle = VK_NULL_HANDLE;
    }
}

bool AppBaseVk::CreateFence()
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    if(vkCreateFence(m_DeviceHandle, &fenceInfo, nullptr, &m_FenceHandle) != VK_SUCCESS)
    {
        Log::Error("Failed to create fence");
        return false;
    }
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.flags = 0;
    if(vkCreateSemaphore(m_DeviceHandle, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS)
    {
        Log::Error("Failed to create semaphore");
        return false;
    }
    
    return true;
}

void AppBaseVk::DestroyFence()
{
    if(m_ImageAvailableSemaphore != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_DeviceHandle, m_ImageAvailableSemaphore, nullptr);
        m_ImageAvailableSemaphore = VK_NULL_HANDLE;
    }
    
    if(m_FenceHandle != VK_NULL_HANDLE)
    {
        vkDestroyFence(m_DeviceHandle, m_FenceHandle, nullptr);
        m_FenceHandle = VK_NULL_HANDLE;
    }
}

void AppBaseVk::ExecuteCommandBuffer(VkSemaphore inWaitSemaphore)
{
    EndCommandList();
    vkResetFences(m_DeviceHandle, 1, &m_FenceHandle);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CmdBufferHandle;
    submitInfo.waitSemaphoreCount = inWaitSemaphore != VK_NULL_HANDLE ? 1 : 0;
    submitInfo.pWaitSemaphores = &inWaitSemaphore;
    submitInfo.pWaitDstStageMask = inWaitSemaphore != VK_NULL_HANDLE ? waitStages : nullptr;
    vkQueueSubmit(m_QueueHandle, 1, &submitInfo, m_FenceHandle);
    vkWaitForFences(m_DeviceHandle, 1, &m_FenceHandle, VK_TRUE, UINT64_MAX);
}

void AppBaseVk::Present()
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSwapchainKHR swapChains[] = { m_SwapChainHandle };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_CurrentIndex;
    vkQueuePresentKHR(m_QueueHandle, &presentInfo);

    // move to next frame
    vkAcquireNextImageKHR(m_DeviceHandle, m_SwapChainHandle, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &m_CurrentIndex);
}