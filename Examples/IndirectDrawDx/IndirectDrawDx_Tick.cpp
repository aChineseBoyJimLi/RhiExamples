#include "IndirectDrawDx.h"

void IndirectDrawDx::UpdateConstants()
{
    CameraData cameraData;
    m_Camera.GetCameraData(cameraData);
    WriteBufferData(m_CameraDataBuffer.Get(), &cameraData, CameraData::GetAlignedByteSizes());

    ViewFrustum viewFrustum;
    m_Camera.GetViewFrustumWorldSpace(viewFrustum);
    ViewFrustumCB viewFrustumCB;
    for(uint32_t i = 0; i < 8; ++i)
    {
        viewFrustumCB.Corners[i] = glm::vec4(viewFrustum.Corners[i], 1.0f);
    }
    WriteBufferData(m_ViewFrustumBuffer.Get(), &viewFrustumCB, ViewFrustumCB::GetAlignedByteSizes());
    
    DirectionalLightData lightData;
    lightData.LightColor = m_Light.Color;
    lightData.LightDirection = m_Light.Transform.GetWorldForward();
    lightData.LightIntensity = m_Light.Intensity;
    WriteBufferData(m_LightDataBuffer.Get(), &lightData, DirectionalLightData::GetAlignedByteSizes());
}

void IndirectDrawDx::Tick()
{
    if(!m_IsRunning)
    {
        return;
    }

    BeginCommandList();
    UpdateConstants();

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_ShaderBoundViewHeap.Get(), m_SamplerHeap.Get() };
    m_CommandList->SetDescriptorHeaps(2, descriptorHeaps);
    
    // Culling Compute Pass
    m_CommandList->SetPipelineState(m_CullingPassPSO.Get());
    m_CommandList->SetComputeRootSignature(m_CullingPassRS.Get());
    m_CommandList->SetComputeRootConstantBufferView(0, m_CameraDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetComputeRootConstantBufferView(1, m_ViewFrustumBuffer->GetGPUVirtualAddress());
    m_CommandList->SetComputeRootShaderResourceView(2, m_InstancesBuffer->GetGPUVirtualAddress());
    m_CommandList->SetComputeRootShaderResourceView(3, m_IndirectCommandsBuffer->GetGPUVirtualAddress());
    D3D12_GPU_DESCRIPTOR_HANDLE outputCommandsUavHanle = m_ShaderBoundViewHeap->GetGPUDescriptorHandleForHeapStart();
    outputCommandsUavHanle.ptr += m_OutputCommandsUavSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_CommandList->SetComputeRootDescriptorTable(4, outputCommandsUavHanle);
    
    // reset the counter in m_ProcessedCommandsBuffer
    m_CommandList->CopyBufferRegion(m_ProcessedCommandsBuffer.Get()
        , sizeof(IndirectCommand) * s_InstancesCount
        , m_ProcessedCommandsResetBuffer.Get()
        , 0
        , sizeof(uint32_t));

    D3D12_RESOURCE_BARRIER preCullingBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_ProcessedCommandsBuffer.Get()
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    m_CommandList->ResourceBarrier(1, &preCullingBarrier);
    m_CommandList->Dispatch(s_InstancesCount / s_ThreadGroupSize, 1, 1);

    D3D12_RESOURCE_BARRIER afterCullingBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_ProcessedCommandsBuffer.Get()
        , D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        , D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

    m_CommandList->ResourceBarrier(1, &afterCullingBarrier);

    // Graphics Pass
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

    m_CommandList->SetGraphicsRootSignature(m_IndirectDrawPassRS.Get());
    m_CommandList->SetPipelineState(m_IndirectDrawPassPSO.Get());

    m_CommandList->SetGraphicsRootConstantBufferView(0, m_CameraDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootConstantBufferView(1, m_LightDataBuffer->GetGPUVirtualAddress());

    D3D12_GPU_DESCRIPTOR_HANDLE mainTextureSrvHandle = m_ShaderBoundViewHeap->GetGPUDescriptorHandleForHeapStart();
    mainTextureSrvHandle.ptr += m_MainTextureSrvBaseSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_CommandList->SetGraphicsRootDescriptorTable(3, mainTextureSrvHandle);
    m_CommandList->SetGraphicsRootShaderResourceView(4, m_MaterialsBuffer->GetGPUVirtualAddress());

    D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = m_SamplerHeap->GetGPUDescriptorHandleForHeapStart();
    samplerHandle.ptr += m_SamplerSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_CommandList->SetGraphicsRootDescriptorTable(5, samplerHandle);

    m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    m_CommandList->IASetIndexBuffer(&m_IndexBufferView);

    m_CommandList->ExecuteIndirect(m_IndirectCommandSignature.Get()
        , s_InstancesCount
        , m_ProcessedCommandsBuffer.Get()
        , 0
        , m_ProcessedCommandsBuffer.Get()
        , m_CommandBufferCounterOffset);
    
    D3D12_RESOURCE_BARRIER postBarriers[2];
    postBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_BackBuffers[m_CurrentIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    postBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_ProcessedCommandsBuffer.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
    m_CommandList->ResourceBarrier(2, postBarriers);
    EndCommandList();    

    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();
    m_SwapChainHandle->Present(0, 0);
    m_CurrentIndex = (m_CurrentIndex + 1) % s_BackBufferCount;
}
