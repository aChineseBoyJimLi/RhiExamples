#include "GraphicsPipelineDx.h"
#include <array>

bool GraphicsPipelineDx::CreateRootSignature()
{
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;

    std::array<CD3DX12_ROOT_PARAMETER, 6> rootParameters;
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // _CameraData
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // _LightData
    rootParameters[2].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // _InstanceData
    CD3DX12_DESCRIPTOR_RANGE descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0, 1); 
    rootParameters[3].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL); // _MainTex[]
    rootParameters[4].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL); // _MaterialData
    CD3DX12_DESCRIPTOR_RANGE samplerRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0); 
    rootParameters[5].InitAsDescriptorTable(1, &samplerRange, D3D12_SHADER_VISIBILITY_PIXEL); // _MainTex_Sampler

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(static_cast<uint32_t>(rootParameters.size()), rootParameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
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

bool GraphicsPipelineDx::CreateShader()
{
    m_VertexShaderBlob = AssetsManager::LoadShaderImmediately("Graphics.vs.bin");
    if(!m_VertexShaderBlob || m_VertexShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load vertex shader");
        return false;
    }
    m_PixelShaderBlob = AssetsManager::LoadShaderImmediately("Graphics.ps.bin");
    if(!m_PixelShaderBlob || m_PixelShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load pixel shader");
        return false;
    }
    
    return true;
}

bool GraphicsPipelineDx::CreatePipelineState()
{
    std::array<D3D12_INPUT_ELEMENT_DESC, 3> inputElements;
    inputElements[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    inputElements[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    inputElements[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
    pipelineDesc.InputLayout = { inputElements.data(), static_cast<uint32_t>(inputElements.size()) };
    pipelineDesc.pRootSignature = m_RootSignature.Get();
    pipelineDesc.VS = CD3DX12_SHADER_BYTECODE(m_VertexShaderBlob->GetData(), m_VertexShaderBlob->GetSize());
    pipelineDesc.PS = CD3DX12_SHADER_BYTECODE(m_PixelShaderBlob->GetData(), m_PixelShaderBlob->GetSize());
    pipelineDesc.NumRenderTargets      = 1;
    pipelineDesc.RTVFormats[0]         = s_BackBufferFormat;
    pipelineDesc.DSVFormat             = s_DepthStencilBufferFormat;
    pipelineDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
    pipelineDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
    pipelineDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
    pipelineDesc.SampleMask            = UINT_MAX;
    pipelineDesc.SampleDesc            = DefaultSampleDesc();
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.NodeMask              = GetNodeMask();

    const HRESULT hr = m_DeviceHandle->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&m_PipelineState));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the pipeline state");
        return false;
    }
    
    return true;
}