#pragma once
#include "Win32Base.h"
#include "Log.h"
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>
#include <iostream>

#include "AssetsManager.h"
#include "Camera.h"
#include "Transform.h"

#define OUTPUT_D3D12_FAILED_RESULT(Re)  if(FAILED(Re))\
    {\
        std::string message = std::system_category().message(Re);\
        Log::Error("[D3D12] Error: %s, In File: %s line %d", message.c_str(), __FILE__, __LINE__);\
    }

void WriteBufferData(ID3D12Resource* inBuffer, const void* inData, uint32_t inSize);

static Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferHelper(ID3D12Device* inDevice
    , size_t inSize
    , D3D12_RESOURCE_STATES initState
    , D3D12_HEAP_TYPE inHeapType
    , D3D12_RESOURCE_FLAGS inFlags);

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureHelper(ID3D12Device* inDevice
    , DXGI_FORMAT inFormat
    , uint32_t inWidth
    , uint32_t inHeight
    , D3D12_RESOURCE_STATES initState
    , D3D12_HEAP_TYPE inHeapType
    , D3D12_RESOURCE_FLAGS inFlags
    , const D3D12_CLEAR_VALUE* inClearValue);

class MeshPipelineDx : public Win32Base
{
public:
    using Win32Base::Win32Base;
    static constexpr D3D_FEATURE_LEVEL  s_FeatureLevel = D3D_FEATURE_LEVEL_12_1;
    static constexpr DXGI_FORMAT        s_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    static constexpr DXGI_FORMAT        s_DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT;
    static constexpr uint32_t           s_BackBufferCount = 2;

    bool Init() override;
    void Tick() override;
    void Shutdown() override;

    void BeginCommandList();
    void EndCommandList();
    
    // not support mGPU so far
    static uint32_t GetNodeMask() { return 0; }
    static uint32_t GetCreationNodeMask() { return 1; }
    static uint32_t GetVisibleNodeMask() { return 1; }

private:
    bool CreateDevice();
    bool CreateCommandQueue();
    bool CreateCommandList();
    bool CreateDescriptorHeaps();
    bool CreateSwapChain();
    bool CreateRootSignature();
    bool CreateShader();
    bool CreatePipelineState();
    bool CreateDepthStencilBuffer();
    bool CreateResources();
    void UpdateConstants();
    void FlushCommandQueue();
    
private:
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

    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_RootSignature;
    std::shared_ptr<AssetsManager::Blob>                m_MeshShaderBlob;
    std::shared_ptr<AssetsManager::Blob>                m_PixelShaderBlob;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_PipelineState;

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_DepthStencilBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE                         m_DsvHandle;

    CameraPerspective                                   m_Camera;
    std::shared_ptr<AssetsManager::Mesh>                m_Mesh;
    Transform                                           m_MeshTransform;
    std::shared_ptr<AssetsManager::Texture>             m_Texture;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TransformDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_CameraDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_VerticesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TexCoordsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_MeshletDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_PackedPrimitiveIndicesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_UniqueVertexIndicesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_MainTexture;
    uint32_t                                            m_GroupCount;

    const uint32_t m_MainTextureSrvSlot {0};       // The slot of main texture srv in the descriptor heap 
    const uint32_t m_SamplerSlot {0};               // The slot of sampler in the descriptor heap 
};