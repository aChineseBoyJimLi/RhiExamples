#include "IndirectDrawDx.h"

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

void WriteBufferData(ID3D12Resource* inBuffer, const void* inData, size_t inSize)
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
    constexpr uint32_t maxSubresourceNum = 1;
    UpdateSubresources<maxSubresourceNum>(inCmdList, dstTexture, stagingBuffer.Get(), 0, 0, subresources.size(), subresources.data());
    return stagingBuffer;
}

bool IndirectDrawDx::CreateDepthStencilBuffer()
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

bool IndirectDrawDx::CreateResources()
{
    
    return true;
}