#include "RayTracingPipelineVk.h"
#include <array>

void RayTracingPipelineVk::UpdateConstants()
{
    CameraData cameraData;
    m_Camera.GetCameraData(cameraData);
    WriteBufferData(m_DeviceHandle, m_CameraBufferMemory, &cameraData, CameraData::GetAlignedByteSizes());

    DirectionalLightData lightData;
    lightData.LightColor = m_MainLight.Color;
    lightData.LightDirection = m_MainLight.Transform.GetWorldForward();
    lightData.LightIntensity = m_MainLight.Intensity;
    WriteBufferData(m_DeviceHandle, m_LightDataMemory, &lightData, DirectionalLightData::GetAlignedByteSizes());
}

void RayTracingPipelineVk::Tick()
{
    if(!m_IsRunning)
    {
        return;
    }

    BeginCommandList();
    UpdateConstants();

    // Ray tracing
    vkCmdBindPipeline(m_CmdBufferHandle, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_PipelineState);
    vkCmdBindDescriptorSets(m_CmdBufferHandle, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);
    vkCmdTraceRaysKHR(m_CmdBufferHandle, &m_RaygenShaderSbtEntry, &m_MissShaderSbtEntry, &m_HitShaderSbtEntry, &m_CallableShaderSbtEntry, m_Capabilities.currentExtent.width, m_Capabilities.currentExtent.height, 1);
    
    // Copy the output image to the back buffer
    std::array<VkImageMemoryBarrier, 2> preCopyBarriers;
    preCopyBarriers[0] = ImageMemoryBarrier(m_OutputImage, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    preCopyBarriers[1] = ImageMemoryBarrier(m_BackBuffers[m_CurrentIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdPipelineBarrier(m_CmdBufferHandle, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(preCopyBarriers.size()), preCopyBarriers.data());
    
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.srcOffset = { 0, 0, 0 };
    copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.dstOffset = { 0, 0, 0 };
    copyRegion.extent = { m_Capabilities.currentExtent.width, m_Capabilities.currentExtent.height, 1 };
    vkCmdCopyImage(m_CmdBufferHandle, m_OutputImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_BackBuffers[m_CurrentIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    
    std::array<VkImageMemoryBarrier, 2> postCopyBarriers;
    postCopyBarriers[0] = ImageMemoryBarrier(m_OutputImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    postCopyBarriers[1] = ImageMemoryBarrier(m_BackBuffers[m_CurrentIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdPipelineBarrier(m_CmdBufferHandle, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(postCopyBarriers.size()), postCopyBarriers.data());
    
    EndCommandList();
    ExecuteCommandBuffer(m_ImageAvailableSemaphore);
    Present();
}