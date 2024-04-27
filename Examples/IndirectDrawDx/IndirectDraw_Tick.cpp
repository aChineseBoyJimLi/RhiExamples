#include "IndirectDrawDx.h"

void IndirectDrawDx::UpdateConstants()
{
    // CameraData cameraData;
    // m_Camera.GetCameraData(cameraData);
    // WriteBufferData(m_CameraDataBuffer.Get(), &cameraData, CameraData::GetAlignedByteSizes());
    //
    // DirectionalLightData lightData;
    // lightData.LightColor = m_Light.Color;
    // lightData.LightDirection = m_Light.Transform.GetWorldForward();
    // lightData.LightIntensity = m_Light.Intensity;
    // WriteBufferData(m_LightDataBuffer.Get(), &lightData, DirectionalLightData::GetAlignedByteSizes());
}

void IndirectDrawDx::Tick()
{
    if(!m_IsRunning)
    {
        return;
    }

    BeginCommandList();
    UpdateConstants();
    
    D3D12_RESOURCE_BARRIER preBarriers;
    preBarriers = CD3DX12_RESOURCE_BARRIER::Transition(m_BackBuffers[m_CurrentIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_CommandList->ResourceBarrier(1, &preBarriers);
    D3D12_VIEWPORT screenViewport;
    screenViewport.TopLeftX = 0;
    screenViewport.TopLeftY = 0;
    screenViewport.Width = static_cast<float>(m_Width);
    screenViewport.Height = static_cast<float>(m_Height);
    screenViewport.MinDepth = 0.0f;
    screenViewport.MaxDepth = 1.0f;
    m_CommandList->RSSetViewports(1, &screenViewport);

    D3D12_RECT scissorRect = { 0, 0, (long)m_Width, (long)m_Height };
    m_CommandList->RSSetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[] = {m_RtvHandles[m_CurrentIndex]};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DsvHandle;
    m_CommandList->OMSetRenderTargets(1, rtvHandle, false, &dsvHandle);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_CommandList->ClearRenderTargetView(rtvHandle[0], clearColor, 0, nullptr);
    m_CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_ShaderBoundViewHeap.Get(), m_SamplerHeap.Get() };
    m_CommandList->SetDescriptorHeaps(2, descriptorHeaps);

    D3D12_RESOURCE_BARRIER postBarriers;
    postBarriers = CD3DX12_RESOURCE_BARRIER::Transition(m_BackBuffers[m_CurrentIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_CommandList->ResourceBarrier(1, &postBarriers);
    EndCommandList();    

    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();
    m_SwapChainHandle->Present(0, 0);
    m_CurrentIndex = (m_CurrentIndex + 1) % s_BackBufferCount;
}
