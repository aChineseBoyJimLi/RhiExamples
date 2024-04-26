#include "RayTracingPipelineDx.h"
#include <array>

Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferHelper(ID3D12Device* inDevice
    , size_t inSize
    , D3D12_RESOURCE_STATES initState
    , D3D12_HEAP_TYPE inHeapType
    , D3D12_RESOURCE_FLAGS inFlags)
{
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = inSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Flags = inFlags;

    const CD3DX12_HEAP_PROPERTIES heapProperties(inHeapType);

    Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
    const HRESULT hr = inDevice->CreateCommittedResource(&heapProperties
        , D3D12_HEAP_FLAG_NONE
        , &resourceDesc
        , initState
        , nullptr
        , IID_PPV_ARGS(&buffer));

    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create buffer");
        return nullptr;
    }
    
    return buffer;
}

void WriteBufferData(ID3D12Resource* inBuffer, const void* inData, uint32_t inSize)
{
    if(inBuffer == nullptr)
    {
        Log::Error("[D3D12] Invalid buffer");
        return;
    }
    uint8_t* mappedData = nullptr;
    const D3D12_RANGE range = {0, inSize};
    inBuffer->Map(0, &range, reinterpret_cast<void**>(&mappedData));
    memcpy(mappedData, inData, inSize);
    inBuffer->Unmap(0, nullptr);
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureHelper(ID3D12Device* inDevice
    , DXGI_FORMAT inFormat
    , uint32_t inWidth
    , uint32_t inHeight
    , D3D12_RESOURCE_STATES initState
    , D3D12_HEAP_TYPE inHeapType
    , D3D12_RESOURCE_FLAGS inFlags
    , const D3D12_CLEAR_VALUE* inClearValue)
{
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = inFormat;
    textureDesc.Width = inWidth;
    textureDesc.Height = inHeight;
    textureDesc.Flags = inFlags;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    const CD3DX12_HEAP_PROPERTIES heapProperties(inHeapType);

    Microsoft::WRL::ComPtr<ID3D12Resource> texture;
    HRESULT hr = inDevice->CreateCommittedResource(&heapProperties
        , D3D12_HEAP_FLAG_NONE
        , &textureDesc
        , initState
        , inClearValue
        , IID_PPV_ARGS(&texture));
    
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create texture");
        return nullptr;
    }

    return texture;
}

static Microsoft::WRL::ComPtr<ID3D12Resource> UploadBuffer(ID3D12Device* inDevice, ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* dstBuffer, const void* inData, size_t inSize)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer = CreateBufferHelper(inDevice
        , inSize
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);

    WriteBufferData(stagingBuffer.Get(), inData, inSize);

    inCmdList->CopyBufferRegion(dstBuffer, 0, stagingBuffer.Get(), 0, inSize);

    return stagingBuffer;
}

static Microsoft::WRL::ComPtr<ID3D12Resource> UploadTexture(ID3D12Device* inDevice, ID3D12GraphicsCommandList* inCmdList, ID3D12Resource* dstTexture, const DirectX::ScratchImage& inImage)
{
    D3D12_RESOURCE_DESC texDesc = dstTexture->GetDesc();
    const size_t numSubresources = inImage.GetImageCount();
    size_t requiredSize;
    inDevice->GetCopyableFootprints(&texDesc, 0, numSubresources, 0, nullptr, nullptr, nullptr, &requiredSize);

    Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer = CreateBufferHelper(inDevice
        , requiredSize
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);
    
    std::vector<D3D12_SUBRESOURCE_DATA> subresources(numSubresources);
    const DirectX::Image* images = inImage.GetImages();
    for(uint32_t i = 0; i < numSubresources; ++i)
    {
        subresources[i].pData = images[i].pixels;
        subresources[i].RowPitch = images[i].rowPitch;
        subresources[i].SlicePitch = images[i].slicePitch;
    }
    constexpr uint32_t maxSubresourceNum = 16;
    UpdateSubresources<maxSubresourceNum>(inCmdList, dstTexture, stagingBuffer.Get(), 0, 0, subresources.size(), subresources.data());
    return stagingBuffer;
}

bool RayTracingPipelineDx::CreateResource()
{
    m_MainLight.Direction = {0.57735f, -0.57735f, 0.57735f};
    m_MainLight.Color = {1.0f, 1.0f, 1.0f};
    m_MainLight.Intensity = 1.0f;
    
    m_Camera.AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    m_Camera.Transform.SetWorldPosition(glm::vec3(0, 0, 4));
    m_Camera.Transform.LookAt(glm::vec3(0, 0, 0));

    m_Mesh = AssetsManager::LoadMeshImmediately("sphere.fbx");
    m_MeshTransform.SetWorldPosition(glm::vec3(0, 0, 0));

    if(m_Mesh == nullptr || m_Mesh->GetMesh() == nullptr)
    {
        Log::Error("Failed to load mesh");
        return false;
    }

    m_Texture = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_01_1024x1024.jpg");

    m_CameraDataBuffer = CreateBufferHelper(m_DeviceHandle.Get(), CameraData::GetAlignedByteSizes(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
    if(m_CameraDataBuffer.Get() == nullptr) return false;
    
    m_LightDataBuffer = CreateBufferHelper(m_DeviceHandle.Get(), DirectionalLightData::GetAlignedByteSizes(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
    if(m_LightDataBuffer.Get() == nullptr) return false;
    
    m_OutputBuffer = CreateTextureHelper(m_DeviceHandle.Get(), DXGI_FORMAT_R8G8B8A8_UNORM, m_Width, m_Height, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr);
    if(m_OutputBuffer.Get() == nullptr) return false;
    
    m_VerticesBuffer = CreateBufferHelper(m_DeviceHandle.Get(), m_Mesh->GetPositionDataByteSize(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_VerticesBuffer.Get() == nullptr) return false;
    
    m_IndicesBuffer = CreateBufferHelper(m_DeviceHandle.Get(), m_Mesh->GetIndicesDataByteSize(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_IndicesBuffer.Get() == nullptr) return false;
    
    m_TexcoordsBuffer = CreateBufferHelper(m_DeviceHandle.Get(), m_Mesh->GetTexCoordDataByteSize(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_TexcoordsBuffer.Get() == nullptr) return false;
    
    m_NormalsBuffer = CreateBufferHelper(m_DeviceHandle.Get(), m_Mesh->GetNormalDataByteSize(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_NormalsBuffer.Get() == nullptr) return false;

    BeginCommandList();

    auto stagingBuffer1 = UploadBuffer(m_DeviceHandle.Get(), m_CommandList.Get(), m_VerticesBuffer.Get(), m_Mesh->GetPositionData(), m_Mesh->GetPositionDataByteSize());
    auto stagingBuffer2 = UploadBuffer(m_DeviceHandle.Get(), m_CommandList.Get(), m_IndicesBuffer.Get(), m_Mesh->GetIndicesData(), m_Mesh->GetIndicesDataByteSize());
    auto stagingBuffer3 = UploadBuffer(m_DeviceHandle.Get(), m_CommandList.Get(), m_TexcoordsBuffer.Get(), m_Mesh->GetTexCoordData(), m_Mesh->GetTexCoordDataByteSize());
    auto stagingBuffer4 = UploadBuffer(m_DeviceHandle.Get(), m_CommandList.Get(), m_NormalsBuffer.Get(), m_Mesh->GetNormalData(), m_Mesh->GetNormalDataByteSize());

    std::array<CD3DX12_RESOURCE_BARRIER, 4> barriers;
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_VerticesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndicesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_TexcoordsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_NormalsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    m_CommandList->ResourceBarrier((uint32_t)barriers.size(), barriers.data());
    
    EndCommandList();
    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();

    D3D12_UNORDERED_ACCESS_VIEW_DESC outputUavDesc{};
    outputUavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    outputUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    outputUavDesc.Texture2D.MipSlice = 0;
    outputUavDesc.Texture2D.PlaneSlice = 0;
    outputUavDesc.Texture2DArray.ArraySize = 1;
    outputUavDesc.Texture2DArray.FirstArraySlice = 0;
    m_DeviceHandle->CreateUnorderedAccessView(m_OutputBuffer.Get(), nullptr, &outputUavDesc, m_ShaderBoundViewHeap->GetCPUDescriptorHandleForHeapStart());
    
    return true;
}

bool RayTracingPipelineDx::CreateBottomLevelAccelStructure()
{
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_VerticesBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(aiVector3D);
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = m_Mesh->GetVertexCount();
    geometryDesc.Triangles.IndexBuffer = m_IndicesBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = m_Mesh->GetIndicesCount();
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;

    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs{};
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;
    bottomLevelInputs.NumDescs = 1;
    bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    m_DeviceHandle->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    if(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes == 0)
    {
        Log::Error("Failed to get bottom level prebuild info");
        return false;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer = CreateBufferHelper(m_DeviceHandle.Get(), bottomLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_BLASBuffer = CreateBufferHelper(m_DeviceHandle.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    if(m_BLASBuffer.Get() == nullptr) return false;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc{};
    bottomLevelBuildDesc.Inputs = bottomLevelInputs;
    bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
    bottomLevelBuildDesc.DestAccelerationStructureData = m_BLASBuffer->GetGPUVirtualAddress();

    BeginCommandList();
    m_CommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_BLASBuffer.Get());
    m_CommandList->ResourceBarrier(1, &barrier);
    EndCommandList();

    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();
    
    return true;
}

bool RayTracingPipelineDx::CreateTopLevelAccelStructure()
{
    std::array<D3D12_RAYTRACING_INSTANCE_DESC, 2> instanceDescs {};
    instanceDescs[0].InstanceID = 0;
    instanceDescs[0].InstanceContributionToHitGroupIndex = 0;
    instanceDescs[0].InstanceMask = 0xFF;
    instanceDescs[0].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDescs[0].AccelerationStructure = m_BLASBuffer->GetGPUVirtualAddress();
    instanceDescs[0].Transform[0][0] = instanceDescs[0].Transform[1][1] = instanceDescs[0].Transform[2][2] = 1;

    instanceDescs[1].InstanceID = 1;
    instanceDescs[1].InstanceContributionToHitGroupIndex = 0;
    instanceDescs[1].InstanceMask = 0xFF;
    instanceDescs[1].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDescs[1].AccelerationStructure = m_BLASBuffer->GetGPUVirtualAddress();
    instanceDescs[1].Transform[0][0] = instanceDescs[1].Transform[1][1] = instanceDescs[1].Transform[2][2] = 1;
    instanceDescs[1].Transform[2][0] = instanceDescs[1].Transform[2][1] = instanceDescs[1].Transform[2][2] = 1;

    uint64_t instanceBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size();
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer = CreateBufferHelper(m_DeviceHandle.Get(), instanceBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
    WriteBufferData(instanceBuffer.Get(), instanceDescs.data(), instanceBufferSize);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs{};
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.InstanceDescs = instanceBuffer->GetGPUVirtualAddress();
    topLevelInputs.NumDescs = (uint32_t)instanceDescs.size();
    topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    m_DeviceHandle->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    if(topLevelPrebuildInfo.ResultDataMaxSizeInBytes == 0)
    {
        Log::Error("Failed to get top level prebuild info");
        return false;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer = CreateBufferHelper(m_DeviceHandle.Get(), topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_TLASBuffer = CreateBufferHelper(m_DeviceHandle.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc{};
    topLevelBuildDesc.Inputs = topLevelInputs;
    topLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
    topLevelBuildDesc.DestAccelerationStructureData = m_TLASBuffer->GetGPUVirtualAddress();

    BeginCommandList();
    m_CommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    EndCommandList();
    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();
    
    return true;
}


#define align_to(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

bool RayTracingPipelineDx::CreateShaderTable()
{
    
    constexpr uint32_t shaderIdentifierCount = 3; // RayGen * 1, Miss * 1, HitGroup * 1
    constexpr size_t shaderTableStride = align_to(64, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    constexpr size_t shaderTableSize = shaderTableStride * shaderIdentifierCount;
    m_ShaderTableBuffer = CreateBufferHelper(m_DeviceHandle.Get(), shaderTableSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);

    void* pData = nullptr;
    m_ShaderTableBuffer->Map(0, nullptr, &pData);
    
    // raygen shader
    memcpy(static_cast<uint8_t*>(pData) + 0 * shaderTableStride
        , m_PipelineStateProperties->GetShaderIdentifier(L"RayGen")
        , shaderTableStride);

    m_RayGenerationShaderRecord.StartAddress = m_ShaderTableBuffer->GetGPUVirtualAddress();
    m_RayGenerationShaderRecord.SizeInBytes = shaderTableStride;

    // RayMiss shader
    memcpy(static_cast<uint8_t*>(pData) + 1 * shaderTableStride
        , m_PipelineStateProperties->GetShaderIdentifier(L"RayMiss")
        , shaderTableStride);

    m_MissShaderRecord.StartAddress = m_ShaderTableBuffer->GetGPUVirtualAddress() + shaderTableStride;
    m_MissShaderRecord.StrideInBytes = shaderTableStride;
    m_MissShaderRecord.SizeInBytes = 1 * shaderTableStride;

    // HitGroup
    memcpy(static_cast<uint8_t*>(pData) + 2 * shaderTableStride
        , m_PipelineStateProperties->GetShaderIdentifier(L"HitGroup")
        , shaderTableStride);

    m_HitGroupRecord.StartAddress = m_ShaderTableBuffer->GetGPUVirtualAddress() + 2 * shaderTableStride;
    m_HitGroupRecord.StrideInBytes = shaderTableStride;
    m_HitGroupRecord.SizeInBytes = 1 * shaderTableStride;
    
    m_ShaderTableBuffer->Unmap(0, nullptr);
    
    return true;
}