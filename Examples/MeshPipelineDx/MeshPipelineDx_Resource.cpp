#include "MeshPipelineDx.h"
#include "AssetsManager.h"

static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferHelper(ID3D12Device* inDevice
    ,uint32_t inSize
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

static void WriteBufferData(ID3D12Resource* inBuffer, const void* inData, uint32_t inSize)
{
    if(inBuffer == nullptr)
    {
        Log::Error("[D3D12] Invalid buffer");
        return;
    }
    void* mappedData = nullptr;
    const D3D12_RANGE range = {0, 0};
    inBuffer->Map(0, &range, &mappedData);
    memcpy(mappedData, inData, inSize);
    inBuffer->Unmap(0, nullptr);
}

static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureHelper(ID3D12Device* inDevice
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

    const D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    m_DeviceHandle->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &dsvDesc, dsvHandle);
    
    return true;
}

void MeshPipelineDx::DestroyDepthStencilBuffer()
{
    const auto refCount = m_DepthStencilBuffer.Reset();
    if(refCount > 0)
        Log::Warning("[D3D12] Depth Stencil Buffer is still in use");
}

bool MeshPipelineDx::CreateResources()
{
    m_Mesh = AssetsManager::LoadMeshImmediately("sphere.fbx");

    if(m_Mesh == nullptr || m_Mesh->GetMesh() == nullptr)
    {
        Log::Error("Failed to load mesh");
        return false;
    }

    m_Texture = AssetsManager::LoadTextureImmediately("checkerboard.png");
    

    std::vector<DirectX::Meshlet> meshlets;
    std::vector<uint8_t> uniqueVertexIndices;
    std::vector<DirectX::MeshletTriangle> packedPrimitiveIndices;

    if(!m_Mesh->ComputeMeshlets(meshlets, uniqueVertexIndices, packedPrimitiveIndices))
        return false;

    m_TransformDataBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , sizeof(TransformData)
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);

    m_CameraDataBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , sizeof(CameraData)
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);
    
    m_VerticesBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , (uint32_t)m_Mesh->GetVertexCount() * sizeof(DirectX::XMFLOAT3)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    m_TexCoordsBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , (uint32_t)m_Mesh->GetVertexCount() * sizeof(DirectX::XMFLOAT2)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    m_MeshletDataBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , (uint32_t)meshlets.size() * sizeof(DirectX::Meshlet)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    m_PackedPrimitiveIndicesBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , (uint32_t)packedPrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    m_UniqueVertexIndicesBuffer = CreateBufferHelper(m_DeviceHandle.Get()
        , (uint32_t)uniqueVertexIndices.size() * sizeof(uint8_t)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    const AssetsManager::TextureDesc& textureDesc = m_Texture->GetTextureDesc();
    // m_MainTexture = CreateTextureHelper(m_DeviceHandle.Get()
    //     , s_MainTextureFormat
    //     , textureDesc.Width
    //     , textureDesc.Height
    //     , D3D12_RESOURCE_STATE_COPY_DEST
    //     , D3D12_HEAP_TYPE_DEFAULT
    //     , D3D12_RESOURCE_FLAG_NONE
    //     , nullptr);
    
    return true;
}

void MeshPipelineDx::DestroyResources()
{
    m_UniqueVertexIndicesBuffer.Reset();
    m_PackedPrimitiveIndicesBuffer.Reset();
    m_MeshletDataBuffer.Reset();
    m_TexCoordsBuffer.Reset();
    m_VerticesBuffer.Reset();
    
    m_Mesh.reset();
}