#include "MeshPipelineVk.h"
#include <array>

void MeshPipelineVk::UpdateConstants()
{
    CameraData cameraData;
    m_Camera.GetCameraData(cameraData);
    WriteBufferData(m_DeviceHandle, m_CameraBufferMemory, &cameraData, CameraData::GetAlignedByteSizes());

    ViewFrustum viewFrustum;
    m_Camera.GetViewFrustumWorldSpace(viewFrustum);
    ViewFrustumCB viewFrustumCB;
    for(uint32_t i = 0; i < 8; ++i)
    {
        viewFrustumCB.Corners[i] = glm::vec4(viewFrustum.Corners[i], 1.0f);
    }
    WriteBufferData(m_DeviceHandle, m_ViewFrustumBufferMemory, &viewFrustumCB, ViewFrustumCB::GetAlignedByteSizes());
}

void MeshPipelineVk::Tick()
{
    if(!m_IsRunning)
    {
        return;
    }

    BeginCommandList();
    UpdateConstants();
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPassHandle;
    renderPassInfo.framebuffer = m_FrameBuffers[m_CurrentIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_Capabilities.currentExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.2f, 0.4f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_CmdBufferHandle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    {
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_Capabilities.currentExtent.width);
        viewport.height = static_cast<float>(m_Capabilities.currentExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(m_CmdBufferHandle, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_Capabilities.currentExtent;
        vkCmdSetScissor(m_CmdBufferHandle, 0, 1, &scissor);

        vkCmdBindPipeline(m_CmdBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineState);
        vkCmdBindDescriptorSets(m_CmdBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

        vkCmdDrawMeshTasksEXT(m_CmdBufferHandle, m_GroupCount, 1, 1);
    }
    vkCmdEndRenderPass(m_CmdBufferHandle);
    EndCommandList();
    
    ExecuteCommandBuffer(m_ImageAvailableSemaphore);
    Present();
}