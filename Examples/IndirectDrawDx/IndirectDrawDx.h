#pragma once

#include "AssetsManager.h"
#include "Camera.h"
#include "Transform.h"
#include "Light.h"
#include <array>

#include "../AppBaseDx.h"


struct IndirectCommand
{
    D3D12_GPU_VIRTUAL_ADDRESS CBV;
    D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    uint32_t Padding;
};

struct ViewFrustumCB
{
    glm::vec4 Corners[8];
    static uint64_t GetAlignedByteSizes()
    {
        return (sizeof(ViewFrustumCB) + 255) & ~255;
    }
};

class IndirectDrawDx : public AppBaseDx
{
public:
    using AppBaseDx::AppBaseDx;

    static constexpr DXGI_FORMAT        s_DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT;
    static constexpr uint32_t           s_InstancesCount = 128;
    static constexpr uint32_t           s_ThreadGroupSize = 128;
    static constexpr uint32_t           s_TexturesCount = 5;

protected:
    bool Init() override;
    void Tick() override;
    void Shutdown() override;

private:
    bool CreateDevice();
    bool CreateRootSignature();
    bool CreateShader();
    bool CreatePipelineState();
    bool CreateDepthStencilBuffer();
    bool CreateResources();
    void UpdateConstants();
    
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_CullingPassRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_IndirectDrawPassRS;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature>      m_IndirectCommandSignature;
    std::shared_ptr<AssetsManager::Blob>                m_ComputeShaderBlob;
    std::shared_ptr<AssetsManager::Blob>                m_VertexShaderBlob;
    std::shared_ptr<AssetsManager::Blob>                m_PixelShaderBlob;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_IndirectDrawPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_CullingPassPSO;

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_DepthStencilBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE                         m_DsvHandle;

    CameraPerspective                                   m_Camera;
    Light                                               m_Light;    
    std::shared_ptr<AssetsManager::Mesh>                m_Mesh;
    std::array<std::shared_ptr<AssetsManager::Texture>, s_TexturesCount>  m_Textures;
    
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_CameraDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_ViewFrustumBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_LightDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_VerticesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_IndicesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_InstancesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_IndirectCommandsBuffer;  // All indirect draw commands
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_ProcessedCommandsBuffer; // remaining indirect draw commands after culling + count of remaining indirect draw commands
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_ProcessedCommandsResetBuffer; // reset the processed commands buffer
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_MaterialsBuffer;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, s_TexturesCount> m_MainTextures;
    size_t                                              m_CommandBufferCounterOffset{0};

    D3D12_VERTEX_BUFFER_VIEW                            m_VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW                             m_IndexBufferView;

    const uint32_t m_OutputCommandsUavSlot{0};      // The slot of _OutputCommands uav in the descriptor heap
    const uint32_t m_MainTextureSrvBaseSlot{1};          // The slot of _MainTex in the descriptor heap
    const uint32_t m_SamplerSlot {0};                // The slot of _MainTex_Sampler in the descriptor heap 
};
