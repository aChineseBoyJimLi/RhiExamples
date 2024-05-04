#pragma once

#include "Win32Base.h"
#include "Log.h"
#include "AssetsManager.h"
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

class StagingBuffer
{
public:
    StagingBuffer(VkDevice inDevice, VkBuffer inBuffer, VkDeviceMemory inMemory)
        : m_Device(inDevice), m_Buffer(inBuffer), m_Memory(inMemory)
    {
        
    }
    
    ~StagingBuffer()
    {
        if(m_Buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_Device, m_Buffer, nullptr);
            m_Buffer = VK_NULL_HANDLE;
        }
        if(m_Memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_Device, m_Memory, nullptr);
            m_Memory = VK_NULL_HANDLE;
        }
    }

private:
    friend class AppBaseVk;
    VkDevice        m_Device;
    VkBuffer        m_Buffer;
    VkDeviceMemory  m_Memory;
};

enum class ERegisterType : uint8_t
{
    ConstantBuffer, // b
    ShaderResource, // t
    UnorderedAccess,// u
    Sampler         // s
};

void WriteBufferData(VkDevice inDevice, VkDeviceMemory inBuffer, const void* inData, size_t inSize, size_t inOffset = 0);
uint32_t GetBindingSlot(ERegisterType registerType, uint32_t inRegisterSlot);
VkDescriptorBufferInfo CreateDescriptorBufferInfo(VkBuffer inBuffer, size_t inSize);
VkDescriptorImageInfo CreateDescriptorImageInfo(VkImageView inImageView, VkSampler inSampler, VkImageLayout inLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
void UpdateBufferDescriptor(VkWriteDescriptorSet& outDescriptor, VkDescriptorSet inSet, VkDescriptorType inType, VkDescriptorBufferInfo* inBufferInfo, uint32_t inBinding);
void UpdateImageDescriptor(VkWriteDescriptorSet& outDescriptor, VkDescriptorSet inSet, VkDescriptorType inType, VkDescriptorImageInfo* inImageInfo, uint32_t inDescriptorCount, uint32_t inBinding);

class AppBaseVk : public Win32Base
{
public:
    using Win32Base::Win32Base;

    static constexpr VkFormat s_BackBufferFormat = VK_FORMAT_R8G8B8A8_UNORM;
    static constexpr uint32_t s_BackBufferCount = 3; // greater than VkSurfaceCapabilitiesKHR::minImageCount, less than or equal to VkSurfaceCapabilitiesKHR::maxImageCount

    void BeginCommandList();
    void EndCommandList();
    int  FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    bool CreateBuffer(size_t inSize
            , VkBufferUsageFlags inUsage
            , VkMemoryPropertyFlags inProperties
            , VkBuffer& outBuffer
            , VkDeviceMemory& outBufferMemory);
    bool CreateTexture(size_t inWidth
            , size_t inHeight
            , VkFormat inFormat
            , VkImageUsageFlags inUsage
            , VkMemoryPropertyFlags inProperties
            , VkImageLayout inLayout
            , VkImage& outImage
            , VkDeviceMemory& outImageMemory);
    bool CreateImageView(VkImage inImage
            , VkFormat inFormat
            , VkImageAspectFlags inAspect
            , VkImageView& outImageView);
    bool CreateSampler(VkSampler& outSampler);

    std::shared_ptr<StagingBuffer> CreateStagingBuffer(size_t inSize);
    std::shared_ptr<StagingBuffer> UploadBuffer(VkBuffer dstBuffer, const void* srcData, size_t size, size_t dstOffset = 0);
    std::shared_ptr<StagingBuffer> UploadTexture(VkImage dstTexture, const void* srcData, size_t width, size_t height, uint32_t channels);

    VkImageMemoryBarrier ImageMemoryBarrier(VkImage inImage
        , VkImageLayout inOldLayout
        , VkImageLayout inNewLayout
        , VkAccessFlagBits inSrcAccessMask
        , VkAccessFlagBits inDstAccessMask
        , VkImageAspectFlags inAspectMask);
    
protected:
    virtual bool CreateDevice() = 0;
    virtual void DestroyDevice() = 0;

    bool CreateFence();
    void DestroyFence();
    bool CreateCommandList();
    void DestroyCommandList();
    bool CreateDescriptorSetPool();
    void DestroyDescriptorSetPool();
    bool CreateSwapChain();
    void DestroySwapChain();
    void ExecuteCommandBuffer(VkSemaphore inWaitSemaphore = VK_NULL_HANDLE);
    void Present();
    
    VkInstance                  m_InstanceHandle;
#if _DEBUG || DEBUG
    VkDebugUtilsMessengerEXT    m_DebugMessenger;
#endif
    VkPhysicalDevice            m_GpuHandle;
    VkPhysicalDeviceProperties  m_GpuProperties;
    VkPhysicalDeviceFeatures    m_GpuFeatures;
    VkPhysicalDeviceMemoryProperties m_GpuMemoryProperties;
    VkSurfaceKHR                m_SurfaceHandle;
    VkDevice                    m_DeviceHandle;
    int                         m_QueueIndex {-1};
    VkQueue                     m_QueueHandle;
    VkCommandPool               m_CmdPoolHandle;
    VkCommandBuffer             m_CmdBufferHandle;
    bool                        m_CmdBufferIsClosed = true;
    VkDescriptorPool            m_DescriptorPoolHandle;
    VkFence                     m_FenceHandle;
    VkSemaphore                 m_ImageAvailableSemaphore;

    VkSurfaceCapabilitiesKHR    m_Capabilities{};
    VkSwapchainKHR              m_SwapChainHandle;
    std::vector<VkImage>        m_BackBuffers;
    std::vector<VkImageView>    m_BackBufferViews;
    uint32_t                    m_CurrentIndex{0};
};

