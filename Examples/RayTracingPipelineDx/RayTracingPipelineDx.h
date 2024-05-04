#pragma once
#include "AssetsManager.h"
#include "Camera.h"
#include "Light.h"
#include "Transform.h"
#include "../AppBaseDx.h"

struct InstanceData
{
    glm::mat4 LocalToWorld;
    glm::mat4 WorldToLocal;
    uint32_t MaterialIndex;
    uint32_t Padding0;
    uint32_t Padding1;
    uint32_t Padding2;
};

struct MaterialData
{
    glm::vec4 Color;
    uint32_t TextureIndex;
    uint32_t IsLambertian; // 0: Mirror reflection, 1: Lambertian
    float Padding0;
    float Padding1;
};

class RayTracingPipelineDx : public AppBaseDx
{
public:
    using AppBaseDx::AppBaseDx;
    static constexpr uint32_t           s_MaxRayRecursionDepth = 3;
    static constexpr DXGI_FORMAT        s_DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT;
    static constexpr uint32_t           s_TriangleMeshInstanceCount = 2;
    static constexpr uint32_t           s_AABBMeshInstanceCount = 1;
    static constexpr uint32_t           s_InstanceCount = s_TriangleMeshInstanceCount + s_AABBMeshInstanceCount;
    static constexpr uint32_t           s_MaterialCount = 2;

protected:
    bool Init() override;
    void Tick() override;
    void Shutdown() override;


    bool CreateDevice();
    bool CreateRootSignature();
    bool CreateShader();
    bool CreatePipelineState();
    bool CreateShaderTable();
    bool CreateScene();
    bool CreateResource();
    bool CreateBottomLevelAccelStructure();
    bool CreateTopLevelAccelStructure();
    void UpdateConstants();
    
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

    Light                                               m_MainLight;
    CameraPerspective                                   m_Camera;
    std::shared_ptr<AssetsManager::Mesh>                m_Mesh;
    std::shared_ptr<AssetsManager::Texture>             m_Texture;
    std::vector<glm::vec2>                              m_TexCoord0Data;
    std::vector<glm::vec4>                              m_NormalData;
    
    std::array<Transform, s_InstanceCount>              m_InstancesTransform;
    std::array<InstanceData, s_InstanceCount>           m_InstancesData;
    std::array<MaterialData, s_MaterialCount>           m_MaterialsData;
    std::array<D3D12_RAYTRACING_AABB, s_AABBMeshInstanceCount> m_AABB;
    

    Microsoft::WRL::ComPtr<ID3D12Resource>              m_CameraDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_LightDataBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_OutputBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_VerticesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_IndicesBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TexcoordsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_NormalsBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_AABBBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_InstanceBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_MaterialBuffer;
    
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_BLASBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_ProceduralGeoBLASBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TLASBuffer;
    const uint32_t                                      m_OutputUavSlot{0}; // The slot of output texture uav in the descriptor heap
    
};