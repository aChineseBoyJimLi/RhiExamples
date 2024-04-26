#include "MeshPipelineDx.h"
#include "AssetsManager.h"
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

bool MeshPipelineDx::CreateDepthStencilBuffer()
{
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = s_DepthStencilBufferFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    m_DepthStencilBuffer = CreateTextureHelper(m_DeviceHandle.Get()
        , s_DepthStencilBufferFormat
        , m_Width
        , m_Height
        , D3D12_RESOURCE_STATE_DEPTH_WRITE
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        , &optClear);

    if(m_DeviceHandle == nullptr)
    {
        Log::Error("[D3D12] Failed to create depth stencil buffer");
        return false;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = s_DepthStencilBufferFormat;
    dsvDesc.Texture2D.MipSlice = 0;

    m_DsvHandle = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    m_DeviceHandle->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &dsvDesc, m_DsvHandle);
    
    return true;
}

bool MeshPipelineDx::CreateResources()
{
    m_Camera.AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    m_Camera.Transform.SetWorldPosition(glm::vec3(0, 1, 4));
    m_Camera.Transform.LookAt(glm::vec3(0, 0, 0));

    m_Mesh = AssetsManager::LoadMeshImmediately("sphere.fbx");
    m_MeshTransform.SetWorldPosition(glm::vec3(0,0,0));

    if(m_Mesh == nullptr || m_Mesh->GetMesh() == nullptr)
    {
        Log::Error("Failed to load mesh");
        return false;
    }

    m_Texture = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_01_1024x1024.jpg");
    
    std::vector<DirectX::Meshlet> meshlets;
    std::vector<uint8_t> uniqueVertexIndices;
    std::vector<DirectX::MeshletTriangle> packedPrimitiveIndices;

    if(!m_Mesh->ComputeMeshlets(meshlets, uniqueVertexIndices, packedPrimitiveIndices))
        return false;

    m_GroupCount = static_cast<uint32_t>(meshlets.size());
    
    m_TransformDataBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , TransformData::GetAlignedByteSizes()
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_TransformDataBuffer.Get()) return false;

    m_CameraDataBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , CameraData::GetAlignedByteSizes()
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_CameraDataBuffer.Get()) return false;
    
    m_VerticesBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , m_Mesh->GetPositionDataByteSize()
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_VerticesBuffer.Get()) return false;

    m_TexCoordsBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , m_Mesh->GetPositionDataByteSize()
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_TexCoordsBuffer.Get()) return false;

    m_MeshletDataBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , m_Mesh->GetTexCoordDataByteSize()
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_MeshletDataBuffer.Get()) return false;

    m_PackedPrimitiveIndicesBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , packedPrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_PackedPrimitiveIndicesBuffer.Get()) return false;

    m_UniqueVertexIndicesBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , uniqueVertexIndices.size() * sizeof(uint8_t)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_UniqueVertexIndicesBuffer.Get()) return false;

    const DirectX::TexMetadata& metadata = m_Texture->GetTextureDesc();
    m_MainTexture = CreateTextureHelper(m_DeviceHandle.Get()
        , metadata.format
        , metadata.width
        , metadata.height
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE
        , nullptr);

    if(!m_MainTexture.Get()) return false;

    BeginCommandList();
    auto stagingBuffer1 = UploadBuffer(m_DeviceHandle.Get(), m_CommandList.Get(), m_VerticesBuffer.Get(), m_Mesh->GetPositionData(), m_Mesh->GetPositionDataByteSize());
    auto stagingBuffer2 = UploadBuffer(m_DeviceHandle.Get(), m_CommandList.Get(), m_TexCoordsBuffer.Get(), m_Mesh->GetTexCoordData(), m_Mesh->GetTexCoordDataByteSize());
    auto stagingBuffer3 = UploadBuffer(m_DeviceHandle.Get(), m_CommandList.Get(), m_MeshletDataBuffer.Get(), meshlets.data(), meshlets.size() * sizeof(DirectX::Meshlet));
    auto stagingBuffer4 = UploadBuffer(m_DeviceHandle.Get(), m_CommandList.Get(), m_PackedPrimitiveIndicesBuffer.Get(), packedPrimitiveIndices.data(), packedPrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle));
    auto stagingBuffer5 = UploadBuffer(m_DeviceHandle.Get(), m_CommandList.Get(), m_UniqueVertexIndicesBuffer.Get(), uniqueVertexIndices.data(), uniqueVertexIndices.size() * sizeof(uint8_t));
    auto stagingBuffer6 = UploadTexture(m_DeviceHandle.Get(), m_CommandList.Get(), m_MainTexture.Get(), m_Texture->GetScratchImage());
    
    std::array<CD3DX12_RESOURCE_BARRIER, 5> barriers;
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_VerticesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_TexCoordsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletDataBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_PackedPrimitiveIndicesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(m_UniqueVertexIndicesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    m_CommandList->ResourceBarrier((uint32_t)barriers.size(), barriers.data());
    
    EndCommandList();
    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();

    // Create Shader Resource View
    D3D12_SHADER_RESOURCE_VIEW_DESC texSrv{};
    texSrv.Format = m_MainTexture->GetDesc().Format;
    texSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    texSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    texSrv.Texture2D.MostDetailedMip = 0;
    texSrv.Texture2D.MipLevels = m_MainTexture->GetDesc().MipLevels;
    texSrv.Texture2D.PlaneSlice = 0;
    texSrv.Texture2D.ResourceMinLODClamp = 0.0f;

    D3D12_CPU_DESCRIPTOR_HANDLE heapHandle = m_ShaderBoundViewHeap->GetCPUDescriptorHandleForHeapStart();
    heapHandle.ptr += m_MainTextureSrvSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_DeviceHandle->CreateShaderResourceView(m_MainTexture.Get(), &texSrv, heapHandle);

    // Create Sampler
    D3D12_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    heapHandle = m_SamplerHeap->GetCPUDescriptorHandleForHeapStart();
    heapHandle.ptr += m_SamplerSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_DeviceHandle->CreateSampler(&samplerDesc, heapHandle);
    
    return true;
}

