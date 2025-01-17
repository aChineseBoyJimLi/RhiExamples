#include "MeshPipelineDx.h"
#include <array>

bool MeshPipelineDx::CreateRootSignature()
{
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    std::array<CD3DX12_ROOT_PARAMETER, 10> rootParameters;
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // _CameraData
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // _ViewFrustum
    rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL); // _MeshInfo
    rootParameters[3].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // _Vertices
    rootParameters[4].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // _TexCoords
    rootParameters[5].InitAsShaderResourceView(2, 0, D3D12_SHADER_VISIBILITY_ALL); // _Meshlets
    rootParameters[6].InitAsShaderResourceView(3, 0, D3D12_SHADER_VISIBILITY_ALL); // _PackedPrimitiveIndices
    rootParameters[7].InitAsShaderResourceView(4, 0, D3D12_SHADER_VISIBILITY_ALL); // _UniqueVertexIndices
    rootParameters[8].InitAsShaderResourceView(5, 0, D3D12_SHADER_VISIBILITY_ALL); // _MeshletCullData
    rootParameters[9].InitAsShaderResourceView(6, 0, D3D12_SHADER_VISIBILITY_ALL); // _InstanceData
    
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(static_cast<uint32_t>(rootParameters.size()), rootParameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to serialize the root signature: %s", static_cast<const char*>(errorBlob->GetBufferPointer()));
        return false;
    }

    hr = m_DeviceHandle->CreateRootSignature(GetNodeMask(),
    signatureBlob->GetBufferPointer()
        , signatureBlob->GetBufferSize()
        , IID_PPV_ARGS(&m_RootSignature));

    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the root signature");
        return false;
    }
    
    return true;
}


bool MeshPipelineDx::CreateShader()
{
    m_AmplificationShaderBlob = AssetsManager::LoadShaderImmediately("VisibleCulling.as.bin");
    if(!m_AmplificationShaderBlob || m_AmplificationShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load task shader");
        return false;
    }
    m_MeshShaderBlob = AssetsManager::LoadShaderImmediately("MeshletViewer.ms.bin");
    if(!m_MeshShaderBlob || m_MeshShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load mesh shader");
        return false;
    }
    m_PixelShaderBlob = AssetsManager::LoadShaderImmediately("SolidColor.ps.bin");
    if(!m_PixelShaderBlob || m_PixelShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load pixel shader");
        return false;
    }
    return true;
}

bool MeshPipelineDx::CreatePipelineState()
{
    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pipelineDesc = {};
    pipelineDesc.pRootSignature        = m_RootSignature.Get();
    pipelineDesc.AS                    = CD3DX12_SHADER_BYTECODE( m_AmplificationShaderBlob->GetData(), m_AmplificationShaderBlob->GetSize() );
    pipelineDesc.MS                    = CD3DX12_SHADER_BYTECODE( m_MeshShaderBlob->GetData(), m_MeshShaderBlob->GetSize() );
    pipelineDesc.PS                    = CD3DX12_SHADER_BYTECODE( m_PixelShaderBlob->GetData(), m_PixelShaderBlob->GetSize() );
    pipelineDesc.NumRenderTargets      = 1;
    pipelineDesc.RTVFormats[0]         = s_BackBufferFormat;
    pipelineDesc.DSVFormat             = s_DepthStencilBufferFormat;
    pipelineDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
    pipelineDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
    pipelineDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
    pipelineDesc.SampleMask            = UINT_MAX;
    pipelineDesc.SampleDesc            = DefaultSampleDesc();
    pipelineDesc.NodeMask              = GetNodeMask();
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(pipelineDesc);
    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
    streamDesc.pPipelineStateSubobjectStream = &psoStream;
    streamDesc.SizeInBytes                   = sizeof(psoStream);
    const HRESULT hr = m_DeviceHandle->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_PipelineState));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the pipeline state");
        return false;
    }
    return true;
}