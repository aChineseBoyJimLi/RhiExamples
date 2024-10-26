#include "IndirectDrawVk.h"
#include <vector>
#include <array>

static const std::vector<const char*> s_ValidationLayerNames = {
    "VK_LAYER_KHRONOS_validation"
};

static const std::vector<const char*> s_InstanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#if _DEBUG || DEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};

static const std::vector<const char*> s_DeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
    , VkDebugUtilsMessageTypeFlagsEXT messageType
    , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
    , void* pUserData)
{
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        Log::Error(TEXT("Validation layer: %s"), pCallbackData->pMessage);
        return VK_FALSE;
    }
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        Log::Warning(TEXT("Validation layer: %s"), pCallbackData->pMessage);
        return VK_SUCCESS;
    }
    else
    {
        Log::Info(TEXT("Validation layer: %s"), pCallbackData->pMessage);
        return VK_SUCCESS;
    }
}

static void LogGpuProperties(const VkPhysicalDeviceProperties& inProperties)
{
    Log::Info("----------------------------------------------------------------");
    Log::Info("GPU Name: %s", inProperties.deviceName);
    Log::Info("API Version: %d.%d.%d", VK_VERSION_MAJOR(inProperties.apiVersion), VK_VERSION_MINOR(inProperties.apiVersion), VK_VERSION_PATCH(inProperties.apiVersion));
    Log::Info("----------------------------------------------------------------");
}

bool IndirectDrawVk::Init()
{
    if(!CreateDevice())
        return false;

    if(!CreateFence())
        return false;

    if(!CreateCommandList())
        return false;

    if(!CreateDescriptorSetPool())
        return false;

    if(!CreateSwapChain())
        return false;
    
    if(!CreateDescriptorLayout())
        return false;

    if(!CreateShader())
        return false;

    if(!CreateRenderPass())
        return false;

    if(!CreatePipelineState())
        return false;

    if(!CreateDepthStencilBuffer())
        return false;

    if(!CreateFrameBuffer())
        return false;

    if(!CreateScene())
        return false;

    if(!CreateResources())
        return false;

    if(!CreateDescriptorSet())
        return false;
    
    return true;    
}

void IndirectDrawVk::Shutdown()
{
    DestroyDescriptorSet();
    DestroyResources();
    DestroyFrameBuffer();
    DestroyDepthStencilBuffer();
    DestroyPipelineState();
    DestroyRenderPass();
    DestroyShader();
    DestroyDescriptorLayout();
    DestroySwapChain();
    DestroyDescriptorSetPool();
    DestroyCommandList();
    DestroyFence();
    DestroyDevice();
}

bool IndirectDrawVk::CreateDevice()
{
    // Create Vulkan instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "IndirectDrawVk";
    appInfo.pEngineName = "No Engine";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo insCreateInfo{};
    insCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    insCreateInfo.pApplicationInfo = &appInfo;
    insCreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_InstanceExtensions.size());
    insCreateInfo.ppEnabledExtensionNames = s_InstanceExtensions.data();

#if _DEBUG || DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = DebugCallback;
    
    insCreateInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayerNames.size());
    insCreateInfo.ppEnabledLayerNames = s_ValidationLayerNames.data();
    insCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
    insCreateInfo.enabledLayerCount = 0;
    insCreateInfo.pNext = nullptr;
#endif

    VkResult result = vkCreateInstance(&insCreateInfo, nullptr, &m_InstanceHandle);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create Vulkan instance");
        return false;
    }

#if _DEBUG || DEBUG
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_InstanceHandle, "vkCreateDebugUtilsMessengerEXT");
    if(vkCreateDebugUtilsMessengerEXT)
    {
        result = vkCreateDebugUtilsMessengerEXT(m_InstanceHandle, &debugCreateInfo, nullptr, &m_DebugMessenger);
        if(result != VK_SUCCESS)
            Log::Warning("Failed to create debug messenger");
    }
#endif

    // Enumerate all gpu devices
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(m_InstanceHandle, &gpuCount, nullptr);
    if( gpuCount == 0)
    {
        Log::Error("No GPU found");
        return false;
    }
    
    std::vector<VkPhysicalDevice> tempPhysicalDevices(gpuCount);
    vkEnumeratePhysicalDevices(m_InstanceHandle, &gpuCount, tempPhysicalDevices.data());
    for(uint32_t i = 0; i < gpuCount; ++i)
    {
        VkPhysicalDeviceProperties gpuProperties;
        vkGetPhysicalDeviceProperties(tempPhysicalDevices[i], &gpuProperties);
        LogGpuProperties(gpuProperties);

        if(gpuProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            m_GpuHandle = tempPhysicalDevices[i];
            m_GpuProperties = gpuProperties;
            Log::Info("[Vulkan] Using gpu: %s", gpuProperties.deviceName);
            break;
        }
    }

    if(m_GpuHandle == VK_NULL_HANDLE)
    {
        Log::Error("Failed to find a discrete GPU");
        return false;
    }

    vkGetPhysicalDeviceFeatures(m_GpuHandle, &m_GpuFeatures);
    vkGetPhysicalDeviceMemoryProperties(m_GpuHandle, &m_GpuMemoryProperties);
    
    // Enumerate all extensions
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(m_GpuHandle, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_GpuHandle, nullptr, &extensionCount, extensions.data());
    for(const auto& extension : extensions)
    {
        Log::Info("[Vulkan] GPU supports extension: %s", extension.extensionName);
    }

    // Enumerate all layers
    uint32_t layerCount = 0;
    vkEnumerateDeviceLayerProperties(m_GpuHandle, &layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateDeviceLayerProperties(m_GpuHandle, &layerCount, layers.data());
    for(const auto& layer : layers)
    {
        Log::Info("[Vulkan] GPU supports layer: %s", layer.layerName);
    }

    // Create Win23 surface
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = m_hWnd;
    surfaceCreateInfo.hinstance = m_hInstance;

    result = vkCreateWin32SurfaceKHR(m_InstanceHandle, &surfaceCreateInfo, nullptr, &m_SurfaceHandle);

    // Enumerate all queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_GpuHandle, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_GpuHandle, &queueFamilyCount, queueFamilies.data());

    // Find a queue family that supports both present, graphics, copy and compute
    constexpr uint32_t queueFlags =  VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    for(uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if((queueFamilies[i].queueFlags & queueFlags) > 0)
        {
            // Check if the queue family supports present
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(m_GpuHandle, i, m_SurfaceHandle, &presentSupport);
            if(presentSupport)
            {
                m_QueueIndex = static_cast<int>(i);
                break;
            }
        }
    }

    if(m_QueueIndex < 0)
    {
        Log::Error("Failed to find a queue family that supports present, graphics, copy and compute");
        return false;
    }

    float queuePriority[1] {1.0f};
    
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_QueueIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = queuePriority;

    // Enable descriptor indexing feature
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures{};
    physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    physicalDeviceDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    physicalDeviceDescriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    physicalDeviceDescriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    physicalDeviceDescriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
    physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
    physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;

    // Create logical device
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features = m_GpuFeatures;
    deviceFeatures2.pNext = &physicalDeviceDescriptorIndexingFeatures;
    
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = nullptr;
    deviceCreateInfo.pNext = &deviceFeatures2;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();
    deviceCreateInfo.enabledLayerCount = 0;

#if _DEBUG || DEBUG
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayerNames.size());
    deviceCreateInfo.ppEnabledLayerNames = s_ValidationLayerNames.data();
#endif

    result = vkCreateDevice(m_GpuHandle, &deviceCreateInfo, nullptr, &m_DeviceHandle);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create logical device");
        return false;
    }

    vkGetDeviceQueue(m_DeviceHandle, m_QueueIndex, 0, &m_QueueHandle);
    
    return true;
}

void IndirectDrawVk::DestroyDevice()
{
    if(m_DeviceHandle != VK_NULL_HANDLE)
    {
        vkDestroyDevice(m_DeviceHandle, nullptr);
        m_DeviceHandle = VK_NULL_HANDLE;
    }
    
    if(m_SurfaceHandle != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_InstanceHandle, m_SurfaceHandle, nullptr);
        m_SurfaceHandle = VK_NULL_HANDLE;
    }
    
    if(m_DebugMessenger != VK_NULL_HANDLE)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_InstanceHandle, "vkDestroyDebugUtilsMessengerEXT");
        if(func != nullptr)
        {
            func(m_InstanceHandle, m_DebugMessenger, nullptr);
            m_DebugMessenger = VK_NULL_HANDLE;
        }
    }
    
    if(m_InstanceHandle != VK_NULL_HANDLE)
    {
        vkDestroyInstance(m_InstanceHandle, nullptr);
        m_InstanceHandle = VK_NULL_HANDLE;
    }
}