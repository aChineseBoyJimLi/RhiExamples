#include "RayTracingPipelineDx.h"

void RayTracingPipelineDx::UpdateConstants()
{
    CameraData cameraData;
    m_Camera.GetCameraData(cameraData);
    WriteBufferData(m_CameraDataBuffer.Get(), &cameraData, CameraData::GetAlignedByteSizes());

    DirectionalLightData lightData;
    lightData.LightColor = m_MainLight.Color;
    lightData.LightDirection = m_MainLight.Direction;
    lightData.LightIntensity = m_MainLight.Intensity;
    WriteBufferData(m_LightDataBuffer.Get(), &lightData, DirectionalLightData::GetAlignedByteSizes());
    
    void* pData = nullptr;
    m_InstanceTransformBuffer->Map(0, nullptr, &pData);
    for(uint32_t i = 0; i < s_MeshInstanceCount; ++i)
    {
        TransformData transformData;
        m_MeshInstances[i].Transform.GetTransformData(transformData);
        memcpy(static_cast<uint8_t*>(pData) + i * sizeof(TransformData)
        , &transformData
        , sizeof(TransformData));
    }
    m_InstanceTransformBuffer->Unmap(0, nullptr);
}

void RayTracingPipelineDx::Tick()
{
    if(!m_IsRunning)
    {
        return;
    }

    UpdateConstants();

    BeginCommandList();
    
    // ray tracing pipeline
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_ShaderBoundViewHeap.Get(), m_SamplerHeap.Get() };
    m_CommandList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

    m_CommandList->SetComputeRootSignature(m_GlobalRootSignature.Get());
    m_CommandList->SetPipelineState1(m_PipelineState.Get());
    m_CommandList->SetComputeRootConstantBufferView(0, m_CameraDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetComputeRootConstantBufferView(1, m_LightDataBuffer->GetGPUVirtualAddress());
    m_CommandList->SetComputeRootShaderResourceView(2, m_TLASBuffer->GetGPUVirtualAddress());

    D3D12_GPU_DESCRIPTOR_HANDLE uavHandle = m_ShaderBoundViewHeap->GetGPUDescriptorHandleForHeapStart();
    uavHandle.ptr += m_OutputUavSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_CommandList->SetComputeRootDescriptorTable(3, uavHandle);

    m_CommandList->SetComputeRootShaderResourceView(4, m_IndicesBuffer->GetGPUVirtualAddress());
    m_CommandList->SetComputeRootShaderResourceView(5, m_VerticesBuffer->GetGPUVirtualAddress());
    m_CommandList->SetComputeRootShaderResourceView(6, m_TexcoordsBuffer->GetGPUVirtualAddress());
    m_CommandList->SetComputeRootShaderResourceView(7, m_NormalsBuffer->GetGPUVirtualAddress());
    m_CommandList->SetComputeRootShaderResourceView(8, m_InstanceTransformBuffer->GetGPUVirtualAddress());

    D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
    dispatchDesc.RayGenerationShaderRecord = m_RayGenerationShaderRecord;
    dispatchDesc.MissShaderTable = m_MissShaderRecord;
    dispatchDesc.HitGroupTable = m_HitGroupRecord;
    dispatchDesc.CallableShaderTable = m_CallableShaderRecord;
    dispatchDesc.Width = m_Width;
    dispatchDesc.Height = m_Height;
    dispatchDesc.Depth = 1;

    m_CommandList->DispatchRays(&dispatchDesc);

    // copy output texture to back buffer
    D3D12_RESOURCE_BARRIER preCopyBarriers[2];
    preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_BackBuffers[m_CurrentIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
    preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_OutputBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_CommandList->ResourceBarrier(2, preCopyBarriers);

    m_CommandList->CopyResource(m_BackBuffers[m_CurrentIndex].Get(), m_OutputBuffer.Get());

    D3D12_RESOURCE_BARRIER postCopyBarriers[2];
    postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_BackBuffers[m_CurrentIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
    postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_OutputBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    m_CommandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);
    EndCommandList();    

    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();
    m_SwapChainHandle->Present(0, 0);
    m_CurrentIndex = (m_CurrentIndex + 1) % s_BackBufferCount;
}