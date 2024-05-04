#include "MeshPipelineDx.h"

void MeshPipelineDx::UpdateConstants()
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
}

void MeshPipelineDx::Tick()
{
    if(!m_IsRunning)
    {
        return;
    }
    
    UpdateConstants();
    BeginCommandList();
    
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
    m_CommandList->SetPipelineState(m_PipelineState.Get());
    m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
    m_CommandList->SetGraphicsRootConstantBufferView(0, m_CameraDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootConstantBufferView(1, m_ViewFrustumBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootConstantBufferView(2, m_MeshInfoBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(3, m_VerticesBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(4, m_TexCoordsBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(5, m_MeshletDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(6, m_PackedPrimitiveIndicesBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(7, m_UniqueVertexIndicesBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(8, m_MeshletCullDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(9, m_InstanceBuffer->GetGPUVirtualAddress());
    
    m_CommandList->DispatchMesh(m_GroupCount, 1, 1);

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
