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
#include "Light.h"
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

class RayTracingPipelineDx : public Win32Base
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
    
    // no mGPU support so far
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
    bool CreateShaderTable();
    bool CreateResource();
    bool CreateBottomLevelAccelStructure();
    bool CreateTopLevelAccelStructure();
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
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_ShaderBoundViewHeap; // CBV, SRV, UAV
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_SamplerHeap;
    
    Microsoft::WRL::ComPtr<IDXGISwapChain1>             m_SwapChainHandle;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>            m_RtvHandles;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_BackBuffers;
    uint32_t                                            m_CurrentIndex{0};

    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_GlobalRootSignature;

    std::shared_ptr<AssetsManager::Blob>                m_RayGenShaderBlob;
    std::shared_ptr<AssetsManager::Blob>                m_MissShadersBlob;
    std::shared_ptr<AssetsManager::Blob>                m_HitGroupShadersBlob;
    Microsoft::WRL::ComPtr<ID3D12StateObject>           m_PipelineState;
    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_PipelineStateProperties;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_ShaderTableBuffer;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE                     m_RayGenerationShaderRecord {};
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE          m_MissShaderRecord {};
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE          m_HitGroupRecord {};
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE          m_CallableShaderRecord {};

    DirectionalLight                                    m_MainLight;
    CameraPerspective                                   m_Camera;
    Transform                                           m_MeshTransform;
    std::shared_ptr<AssetsManager::Mesh>                m_Mesh;
    std::shared_ptr<AssetsManager::Texture>             m_Texture;

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_CameraDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_LightDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_OutputBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_VerticesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_IndicesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TexcoordsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_NormalsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_BLASBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TLASBuffer;
    const uint32_t                                      m_OutputUavSlot{0}; // The slot of output texture uav in the descriptor heap 
};