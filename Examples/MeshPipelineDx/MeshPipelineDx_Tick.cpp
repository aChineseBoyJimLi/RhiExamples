#include "MeshPipelineDx.h"

static float s_YawDegree = 0;
static float s_AngularVelocity = 0.5f;

void MeshPipelineDx::UpdateConstants()
{
    s_YawDegree = glm::clamp((s_YawDegree + GetDeltaTime()) * s_AngularVelocity, 0.0f, 360.0f) ;
    
    TransformData transformData;
    m_MeshTransform.GetTransformData(transformData);
    m_MeshTransform.Yaw(s_YawDegree);

    CameraData cameraData;
    m_Camera.GetCameraData(cameraData);
    
    WriteBufferData(m_TransformDataBuffer.Get(), &transformData, TransformData::GetAlignedByteSizes());
    WriteBufferData(m_CameraDataBuffer.Get(), &cameraData, CameraData::GetAlignedByteSizes());
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

    D3D12_RECT scissorRect = { 0, 0, m_Width, m_Height };
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
    m_CommandList->SetGraphicsRootConstantBufferView(0, m_TransformDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootConstantBufferView(1, m_CameraDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(2, m_VerticesBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(3, m_TexCoordsBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(4, m_MeshletDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(5, m_PackedPrimitiveIndicesBuffer->GetGPUVirtualAddress());
    m_CommandList->SetGraphicsRootShaderResourceView(6, m_UniqueVertexIndicesBuffer->GetGPUVirtualAddress());
    
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandle = m_ShaderBoundViewHeap->GetGPUDescriptorHandleForHeapStart();
    textureSrvHandle.ptr += m_MainTextureSrvSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_CommandList->SetGraphicsRootDescriptorTable(7, textureSrvHandle);
    
    D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = m_SamplerHeap->GetGPUDescriptorHandleForHeapStart();
    samplerHandle.ptr += m_SamplerSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_CommandList->SetGraphicsRootDescriptorTable(8, samplerHandle);
    
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
