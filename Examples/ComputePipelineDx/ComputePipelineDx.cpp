#include "ComputePipelineDx.h"

static void LogAdapterDesc(const DXGI_ADAPTER_DESC1& inDesc)
{
    std::wstring adapterDesc(inDesc.Description);
    std::string name(adapterDesc.begin(), adapterDesc.end());
    Log::Info("----------------------------------------------------------------");
    Log::Info("[D3D12] Adapter Description: %s", name.c_str());
    Log::Info("[D3D12] Adapter Dedicated Video Memory: %d MB", inDesc.DedicatedVideoMemory >> 20);
    Log::Info("[D3D12] Adapter Dedicated System Memory: %d MB", inDesc.DedicatedSystemMemory >> 20);
    Log::Info("[D3D12] Adapter Shared System Memory: %d MB", inDesc.SharedSystemMemory >> 20);
    Log::Info("----------------------------------------------------------------");
}

bool ComputePipelineDx::Init()
{
    if(!CreateDevice())
        return false;
    
    if(!CreateCommandQueue())
        return false;

    if(!CreateCommandList())
        return false;

    if(!CreateDescriptorHeaps())
        return false;

    if(!CreateSwapChain())
        return false;

    if(!CreateDepthStencilBuffer())
        return false;

    if(!CreateRootSignature())
        return false;

    if(!CreateShader())
        return false;

    if(!CreatePipelineState())
        return false;

    if(!CreateResources())
        return false;
    
    return true;
}

void ComputePipelineDx::Shutdown()
{
    FlushCommandQueue();
}

bool ComputePipelineDx::CreateDevice()
{
    bool allowDebugging = false;
#if _DEBUG || DEBUG
    allowDebugging = true;
    // Enable the D3D12 debug layer
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        debugController->EnableDebugLayer();
    else
        Log::Warning("Failed to enable the D3D12 debug layer");
#endif
    
    const uint32_t factoryFlags = allowDebugging ? DXGI_CREATE_FACTORY_DEBUG : 0;
    HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_FactoryHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }

    // Enumerate the adapters and create the device
    Microsoft::WRL::ComPtr<IDXGIAdapter1> tempAdapter;
    for (uint32_t i = 0; m_FactoryHandle->EnumAdapters1(i, &tempAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 Desc;
        if(SUCCEEDED(tempAdapter->GetDesc1(&Desc)))
        {
            LogAdapterDesc(Desc);
            // Skip software adapters
            if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            Microsoft::WRL::ComPtr<ID3D12Device5> TempDevice;
            if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), s_FeatureLevel, IID_PPV_ARGS(&TempDevice))))
            {
                m_AdapterHandle = tempAdapter;
                m_DeviceHandle = TempDevice;
                std::wstring adapterDesc(Desc.Description);
                std::string name(adapterDesc.begin(), adapterDesc.end());
                Log::Info("[D3D12] Using adapter: %s", name.c_str());
                break;
            }
        }
    }

    if(m_DeviceHandle.Get() == nullptr)
    {
        Log::Error("Failed to find a device supporting Mesh Shaders");
        return false;
    }
    
    return true;
}

bool ComputePipelineDx::CreateCommandQueue()
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

bool ComputePipelineDx::CreateCommandList()
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

void ComputePipelineDx::BeginCommandList()
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

void ComputePipelineDx::EndCommandList()
{
    if(!m_CommandListIsClosed)
    {
        m_CommandList->Close();
        m_CommandListIsClosed = true;
    }
}

bool ComputePipelineDx::CreateDescriptorHeaps()
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
    shaderBoundViewHeapDesc.NumDescriptors = 20;
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
    samplerHeapDesc.NumDescriptors = 20;
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

bool ComputePipelineDx::CreateSwapChain()
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

void ComputePipelineDx::FlushCommandQueue()
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