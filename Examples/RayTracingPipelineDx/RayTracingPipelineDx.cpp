#include "RayTracingPipelineDx.h"

bool RayTracingPipelineDx::Init()
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

    if(!CreateRootSignature())
        return false;

    if(!CreateShader())
        return false;
    
    if(!CreatePipelineState())
        return false;

    if(!CreateShaderTable())
        return false;

    if(!CreateScene())
        return false;
    
    if(!CreateResource())
        return false;

    if(!CreateBottomLevelAccelStructure())
        return false;

    if(!CreateTopLevelAccelStructure())
        return false;
    
    return true;
}

void RayTracingPipelineDx::Shutdown()
{
    
}

bool RayTracingPipelineDx::CreateDevice()
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
                D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5; // Check for ray tracing support
                if(SUCCEEDED(TempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7))))
                {
                    if(features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
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
        }
    }

    if(m_DeviceHandle.Get() == nullptr)
    {
        Log::Error("Failed to find a device supporting ray tracing pipeline");
        return false;
    }
    
    return true;
}