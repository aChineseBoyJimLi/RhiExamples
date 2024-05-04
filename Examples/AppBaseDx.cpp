#include "AppBaseDx.h"
#include "DirectXTex.h"

void WriteBufferData(ID3D12Resource* inBuffer, const void* inData, size_t inSize, size_t inOffset)
{
    if(inBuffer == nullptr)
    {
        Log::Error("[D3D12] Invalid buffer");
        return;
    }
    uint8_t* mappedData = nullptr;
    const D3D12_RANGE range = {inOffset, inOffset + inSize};
    inBuffer->Map(0, &range, reinterpret_cast<void**>(&mappedData));
    memcpy(mappedData, inData, inSize);
    inBuffer->Unmap(0, nullptr);
}

void AppBaseDx::LogAdapterDesc(const DXGI_ADAPTER_DESC1& inDesc)
{
    std::wstring adapterDesc(inDesc.Description);
    std::string name(adapterDesc.begin(), adapterDesc.end());
    Log::Info("----------------------------------------------------------------", name.c_str());
    Log::Info("[D3D12] Adapter Description: %s", name.c_str());
    Log::Info("[D3D12] Adapter Dedicated Video Memory: %d MB", inDesc.DedicatedVideoMemory >> 20);
    Log::Info("[D3D12] Adapter Dedicated System Memory: %d MB", inDesc.DedicatedSystemMemory >> 20);
    Log::Info("[D3D12] Adapter Shared System Memory: %d MB", inDesc.SharedSystemMemory >> 20);
    Log::Info("----------------------------------------------------------------", name.c_str());
}

bool AppBaseDx::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.NodeMask = GetNodeMask();
    HRESULT hr = m_DeviceHandle->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CommandQueueHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the command queue");
        return false;
    }
    
    hr = m_DeviceHandle->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the fence");
        return false;
    }
    return true;
}

bool AppBaseDx::CreateCommandList()
{
    HRESULT hr = m_DeviceHandle->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the command allocator");
        return false;
    }

    hr = m_DeviceHandle->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the command list");
        return false;
    }
    m_CommandListIsClosed = false;
    return true;
}

bool AppBaseDx::CreateDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.NumDescriptors = s_BackBufferCount;
    rtvHeapDesc.NodeMask = GetNodeMask();
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT hr = m_DeviceHandle->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the RTV descriptor heap");
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.NodeMask = GetNodeMask();
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    hr = m_DeviceHandle->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DsvHeap));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the DSV descriptor heap");
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC shaderBoundViewHeapDesc{};
    shaderBoundViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    shaderBoundViewHeapDesc.NumDescriptors = 8;
    shaderBoundViewHeapDesc.NodeMask = GetNodeMask();
    shaderBoundViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = m_DeviceHandle->CreateDescriptorHeap(&shaderBoundViewHeapDesc, IID_PPV_ARGS(&m_ShaderBoundViewHeap));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the CBV, SRV, UAV descriptor heap");
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc{};
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.NumDescriptors = 8;
    samplerHeapDesc.NodeMask = GetNodeMask();
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = m_DeviceHandle->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&m_SamplerHeap));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the sampler descriptor heap");
        return false;
    }

    return true;
}

bool AppBaseDx::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = m_Width;
    swapChainDesc.Height = m_Height;
    swapChainDesc.BufferCount = s_BackBufferCount;
    swapChainDesc.Format = s_BackBufferFormat;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.Stereo = false;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc{};
    fullscreenDesc.RefreshRate.Numerator = 0;
    fullscreenDesc.RefreshRate.Denominator = 1;
    fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    fullscreenDesc.Windowed = true;

    HRESULT hr = m_FactoryHandle->CreateSwapChainForHwnd(m_CommandQueueHandle.Get(), m_hWnd, &swapChainDesc, &fullscreenDesc, nullptr, &m_SwapChainHandle);
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the swap chain");
        return false;
    }

    // Create the render target views
    m_RtvHandles.resize(s_BackBufferCount);
    m_BackBuffers.resize(s_BackBufferCount);
    const uint32_t incrementSize = m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = s_BackBufferFormat;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    
    for(uint32_t i = 0; i < s_BackBufferCount; ++i)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
        hr = m_SwapChainHandle->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        if(FAILED(hr))
        {
            OUTPUT_D3D12_FAILED_RESULT(hr)
            Log::Error("[D3D12] Failed to get the swap chain buffer");
            return false;
        }
        m_BackBuffers[i] = backBuffer;
        m_RtvHandles[i] = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();
        m_RtvHandles[i].ptr += i * incrementSize;
        m_DeviceHandle->CreateRenderTargetView(backBuffer.Get(), &rtvDesc, m_RtvHandles[i]);
    }

    return true;
}

void AppBaseDx::BeginCommandList()
{
    if(m_CommandList == nullptr)
        return;
    if(m_CommandListIsClosed)
    {
        m_CommandAllocator->Reset();
        m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
        m_CommandListIsClosed = false;
    }
}

void AppBaseDx::EndCommandList()
{
    if(!m_CommandListIsClosed)
    {
        m_CommandList->Close();
        m_CommandListIsClosed = true;
    }
}

void AppBaseDx::FlushCommandQueue()
{
    if(m_CommandQueueHandle.Get())
    {
        m_Fence->Signal(0);
        uint64_t completeValue = 1;
        m_CommandQueueHandle->Signal(m_Fence.Get(), completeValue);
        if(m_Fence->GetCompletedValue() < completeValue)
        {
            HANDLE eventHandle = CreateEventEx(nullptr, "Wait For GPU", false, EVENT_ALL_ACCESS);
            if(eventHandle != nullptr)
            {
                m_Fence->SetEventOnCompletion(completeValue, eventHandle);
                WaitForSingleObject(eventHandle, INFINITE);
                CloseHandle(eventHandle);
            }
        }
    }
}

Microsoft::WRL::ComPtr<ID3D12Resource> AppBaseDx::CreateTexture(DXGI_FORMAT inFormat
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
    HRESULT hr = m_DeviceHandle->CreateCommittedResource(&heapProperties
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

Microsoft::WRL::ComPtr<ID3D12Resource> AppBaseDx::CreateBuffer(size_t inSize
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
    const HRESULT hr = m_DeviceHandle->CreateCommittedResource(&heapProperties
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

Microsoft::WRL::ComPtr<ID3D12Resource> AppBaseDx::UploadBuffer(ID3D12Resource* dstBuffer, const void* inData, size_t inSize, size_t inDstOffset)
{
    Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer = CreateBuffer(inSize
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);

    WriteBufferData(stagingBuffer.Get(), inData, inSize);

    m_CommandList->CopyBufferRegion(dstBuffer, inDstOffset, stagingBuffer.Get(), 0, inSize);

    return stagingBuffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource> AppBaseDx::UploadTexture(ID3D12Resource* dstTexture, const DirectX::ScratchImage& inImage)
{
    D3D12_RESOURCE_DESC texDesc = dstTexture->GetDesc();
    const size_t numSubresources = inImage.GetImageCount();
    size_t requiredSize;
    m_DeviceHandle->GetCopyableFootprints(&texDesc, 0, numSubresources, 0, nullptr, nullptr, nullptr, &requiredSize);

    Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer = CreateBuffer(requiredSize
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
    UpdateSubresources<maxSubresourceNum>(m_CommandList.Get(), dstTexture, stagingBuffer.Get(), 0, 0, subresources.size(), subresources.data());
    return stagingBuffer;
}