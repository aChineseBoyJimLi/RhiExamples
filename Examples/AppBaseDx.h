#pragma once

#include "Win32Base.h"
#include "Log.h"
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>
#include <iostream>
#include <array>

#define OUTPUT_D3D12_FAILED_RESULT(Re)  if(FAILED(Re))\
    {\
        std::string message = std::system_category().message(Re);\
        Log::Error("[D3D12] Error: %s, In File: %s line %d", message.c_str(), __FILE__, __LINE__);\
    }

namespace DirectX
{
    class ScratchImage;
}

void WriteBufferData(ID3D12Resource* inBuffer, const void* inData, size_t inSize, size_t inOffset = 0);

class AppBaseDx : public Win32Base
{
public:
    using Win32Base::Win32Base;
    static constexpr D3D_FEATURE_LEVEL  s_FeatureLevel = D3D_FEATURE_LEVEL_12_1;
    static constexpr DXGI_FORMAT        s_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    static constexpr uint32_t           s_BackBufferCount = 2;

    void BeginCommandList();
    void EndCommandList();

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(size_t inSize
        , D3D12_RESOURCE_STATES initState
        , D3D12_HEAP_TYPE inHeapType
        , D3D12_RESOURCE_FLAGS inFlags);

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateTexture(DXGI_FORMAT inFormat
        , uint32_t inWidth
        , uint32_t inHeight
        , D3D12_RESOURCE_STATES initState
        , D3D12_HEAP_TYPE inHeapType
        , D3D12_RESOURCE_FLAGS inFlags
        , const D3D12_CLEAR_VALUE* inClearValue);

    Microsoft::WRL::ComPtr<ID3D12Resource> UploadBuffer(ID3D12Resource* dstBuffer
        , const void* inData
        , size_t inSize
        , size_t inDstOffset = 0);

    Microsoft::WRL::ComPtr<ID3D12Resource> UploadTexture(ID3D12Resource* dstTexture, const DirectX::ScratchImage& inImage);

    // no mGPU support so far
    static uint32_t GetNodeMask() { return 0; }
    static uint32_t GetCreationNodeMask() { return 1; }
    static uint32_t GetVisibleNodeMask() { return 1; }

    static void LogAdapterDesc(const DXGI_ADAPTER_DESC1& inDesc);

protected:
    bool CreateCommandQueue();
    bool CreateCommandList();
    bool CreateDescriptorHeaps();
    bool CreateSwapChain();
    void FlushCommandQueue();
    
    Microsoft::WRL::ComPtr<IDXGIFactory2>               m_FactoryHandle;
    Microsoft::WRL::ComPtr<IDXGIAdapter1>               m_AdapterHandle;
    Microsoft::WRL::ComPtr<ID3D12Device5>               m_DeviceHandle;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_CommandQueueHandle;
    Microsoft::WRL::ComPtr<ID3D12Fence>                 m_Fence;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_CommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>  m_CommandList;
    bool                                                m_CommandListIsClosed = false;         

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_RtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_DsvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_ShaderBoundViewHeap; // CBV, SRV, UAV
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_SamplerHeap;

    Microsoft::WRL::ComPtr<IDXGISwapChain1>             m_SwapChainHandle;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>            m_RtvHandles;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_BackBuffers;
    uint32_t                                            m_CurrentIndex{0};
    
};