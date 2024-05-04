#pragma once

#include "AssetsManager.h"
#include "Camera.h"
#include "Transform.h"
#include "../AppBaseDx.h"

struct MeshInfo
{
    uint32_t VertexCount;
    uint32_t IndexCount;
    uint32_t MeshletCount;
    uint32_t InstanceCount;

    static uint64_t GetAlignedByteSizes()
    {
        return (sizeof(MeshInfo) + 255) & ~255;
    }
};

struct InstanceData
{
    glm::mat4 LocalToWorld;
    glm::mat4 WorldToLocal;
};

struct ViewFrustumCB
{
    glm::vec4 Corners[8];
    static uint64_t GetAlignedByteSizes()
    {
        return (sizeof(ViewFrustumCB) + 255) & ~255;
    }
};

class MeshPipelineDx : public AppBaseDx
{
public:
    using AppBaseDx::AppBaseDx;
    static constexpr uint32_t       s_InstanceCountX = 8, s_InstanceCountY = 8, s_InstanceCountZ = 8;
    static constexpr uint32_t       s_InstancesCount = s_InstanceCountX * s_InstanceCountY * s_InstanceCountZ;
    static constexpr uint32_t       s_ASThreadGroupSize = 32;
    static constexpr uint32_t       s_MaterialCount = 100;
    static constexpr DXGI_FORMAT    s_DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT;

protected:
    bool Init() override;
    void Tick() override;
    void Shutdown() override;
    
    bool CreateDevice();
    bool CreateRootSignature();
    bool CreateShader();
    bool CreatePipelineState();
    bool CreateDepthStencilBuffer();
    bool CreateScene();
    bool CreateResources();
    void UpdateConstants();
    
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_RootSignature;
    std::shared_ptr<AssetsManager::Blob>                m_AmplificationShaderBlob;
    std::shared_ptr<AssetsManager::Blob>                m_MeshShaderBlob;
    std::shared_ptr<AssetsManager::Blob>                m_PixelShaderBlob;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_PipelineState;

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_DepthStencilBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE                         m_DsvHandle;
    
    CameraPerspective                                   m_Camera;
    MeshInfo                                            m_MeshInfo;
    std::vector<glm::vec4>                              m_PositionData;
    std::vector<glm::vec2>                              m_TexCoord0Data;
    std::shared_ptr<AssetsManager::Mesh>                m_Mesh;
    std::vector<DirectX::Meshlet>                       m_Meshlets;
    std::vector<uint8_t>                                m_UniqueVertexIndices;
    std::vector<DirectX::MeshletTriangle>               m_PackedPrimitiveIndices;
    std::vector<glm::vec4>                              m_MeshletCullData;
    std::array<InstanceData, s_InstancesCount>          m_InstancesData;
    uint32_t                                            m_GroupCount;
    
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_CameraDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_ViewFrustumBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_MeshInfoBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_VerticesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TexCoordsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_MeshletDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_PackedPrimitiveIndicesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_UniqueVertexIndicesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_MeshletCullDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_InstanceBuffer;
};
