#pragma once

#include "../AppBaseDx.h"
#include "AssetsManager.h"
#include "Camera.h"
#include "Transform.h"
#include "Light.h"

class GraphicsPipelineDx : public AppBaseDx
{
public:
    using AppBaseDx::AppBaseDx;
    static constexpr DXGI_FORMAT        s_DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT;
    static constexpr uint32_t           s_InstancesCount = 500;
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
    
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_RootSignature;
    std::shared_ptr<AssetsManager::Blob>                m_VertexShaderBlob;
    std::shared_ptr<AssetsManager::Blob>                m_PixelShaderBlob;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_PipelineState;

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_DepthStencilBuffer;
    D3D12_CPU_DESCRIPTOR_HANDLE                         m_DsvHandle;

    CameraPerspective                                   m_Camera;
    Light                                               m_Light;    
    std::shared_ptr<AssetsManager::Mesh>                m_Mesh;
    std::array<std::shared_ptr<AssetsManager::Texture>, s_TexturesCount>  m_Textures;
    
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_CameraDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_LightDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_VerticesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_IndicesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_InstancesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_MaterialsBuffer;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, s_TexturesCount> m_MainTextures;

    D3D12_VERTEX_BUFFER_VIEW                            m_VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW                             m_IndexBufferView;
};
