#include "IndirectDrawDx.h"

bool IndirectDrawDx::CreateRootSignature()
{
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;

    // create culling pass root signature
    std::array<CD3DX12_ROOT_PARAMETER1, 5> cullingPassRP;
    cullingPassRP[0].InitAsConstantBufferView(0, 0); // _CameraData
    cullingPassRP[1].InitAsConstantBufferView(1, 0); // _ViewFrustum
    cullingPassRP[2].InitAsShaderResourceView(0, 0); // _InstancesData
    cullingPassRP[3].InitAsShaderResourceView(1, 0); // _InputCommands
    CD3DX12_DESCRIPTOR_RANGE1 range(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    cullingPassRP[4].InitAsDescriptorTable(1, &range); // _OutputCommands

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC cullingPassRSDesc;
    cullingPassRSDesc.Init_1_1(static_cast<uint32_t>(cullingPassRP.size()), cullingPassRP.data());
    
    HRESULT hr = D3DX12SerializeVersionedRootSignature(&cullingPassRSDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signatureBlob, &errorBlob);
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to serialize the culling pass root signature: %s", static_cast<const char*>(errorBlob->GetBufferPointer()));
        return false;
    }

    hr = m_DeviceHandle->CreateRootSignature(GetNodeMask()
        , signatureBlob->GetBufferPointer()
        , signatureBlob->GetBufferSize()
        , IID_PPV_ARGS(&m_CullingPassRS));

    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the culling pass root signature");
        return false;
    }

    // create graphics pass root signature
    std::array<CD3DX12_ROOT_PARAMETER1, 6> graphicsPassRP;
    graphicsPassRP[0].InitAsConstantBufferView(0, 0); // _CameraData
    graphicsPassRP[1].InitAsConstantBufferView(1, 0); // _LightData
    graphicsPassRP[2].InitAsConstantBufferView(2, 0 , D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX); // _InstanceData
    CD3DX12_DESCRIPTOR_RANGE1 range1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, s_TexturesCount, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    graphicsPassRP[3].InitAsDescriptorTable(1, &range1); // _MainTex[]
    graphicsPassRP[4].InitAsShaderResourceView(0); // _MaterialData
    CD3DX12_DESCRIPTOR_RANGE1 range2(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    graphicsPassRP[5].InitAsDescriptorTable(1, &range2); // _MainTex_Sampler
    
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC graphicsPassRSDesc;
    graphicsPassRSDesc.Init_1_1(static_cast<uint32_t>(graphicsPassRP.size()), graphicsPassRP.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    hr = D3DX12SerializeVersionedRootSignature(&graphicsPassRSDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signatureBlob, &errorBlob);
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to serialize the graphcis pass root signature: %s", static_cast<const char*>(errorBlob->GetBufferPointer()));
        return false;
    }

    hr = m_DeviceHandle->CreateRootSignature(GetNodeMask()
        , signatureBlob->GetBufferPointer()
        , signatureBlob->GetBufferSize()
        , IID_PPV_ARGS(&m_IndirectDrawPassRS));

    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the graphics pass root signature");
        return false;
    }

    // The command signature of IndirectCommand
    D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[2];
    argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
    argumentDescs[0].ConstantBufferView.RootParameterIndex = 2;         // IndirectCommand::CBV -> _InstanceData
    argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;  // IndirectCommand::DrawIndexedArguments
    
    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc{};
    commandSignatureDesc.ByteStride = sizeof(IndirectCommand);
    commandSignatureDesc.NumArgumentDescs = 2;
    commandSignatureDesc.pArgumentDescs = argumentDescs;
    commandSignatureDesc.NodeMask = GetNodeMask();

    hr = m_DeviceHandle->CreateCommandSignature(&commandSignatureDesc, m_IndirectDrawPassRS.Get(), IID_PPV_ARGS(&m_IndirectCommandSignature));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the indirect command signature");
        return false;
    }
    
    return true;
}

bool IndirectDrawDx::CreateShader()
{
    m_VertexShaderBlob = AssetsManager::LoadShaderImmediately("IndirectDraw.vs.bin");
    if(!m_VertexShaderBlob || m_VertexShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load vertex shader");
        return false;
    }
    
    m_PixelShaderBlob = AssetsManager::LoadShaderImmediately("IndirectDraw.ps.bin");
    if(!m_PixelShaderBlob || m_PixelShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load pixel shader");
        return false;
    }

    m_ComputeShaderBlob = AssetsManager::LoadShaderImmediately("VisibleCullingDx.cs.bin");
    if(!m_ComputeShaderBlob || m_ComputeShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load compute shader");
        return false;
    }
    
    return true;
}

bool IndirectDrawDx::CreatePipelineState()
{
    // culling pass pso
    D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc{};
    computePsoDesc.pRootSignature = m_CullingPassRS.Get();
    computePsoDesc.NodeMask = GetNodeMask();
    computePsoDesc.CS = CD3DX12_SHADER_BYTECODE(m_ComputeShaderBlob->GetData(), m_ComputeShaderBlob->GetSize());
    
    HRESULT hr = m_DeviceHandle->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(&m_CullingPassPSO));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the compute pipeline state");
        return false;
    }

    std::array<D3D12_INPUT_ELEMENT_DESC, 3> inputElements;
    inputElements[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    inputElements[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    inputElements[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

    // graphics pass pso
    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPsoDesc{};
    graphicsPsoDesc.InputLayout = {inputElements.data(), static_cast<uint32_t>(inputElements.size())};
    graphicsPsoDesc.pRootSignature = m_IndirectDrawPassRS.Get();
    graphicsPsoDesc.VS = CD3DX12_SHADER_BYTECODE(m_VertexShaderBlob->GetData(), m_VertexShaderBlob->GetSize());
    graphicsPsoDesc.PS = CD3DX12_SHADER_BYTECODE(m_PixelShaderBlob->GetData(), m_PixelShaderBlob->GetSize());
    graphicsPsoDesc.NumRenderTargets      = 1;
    graphicsPsoDesc.RTVFormats[0]         = s_BackBufferFormat;
    graphicsPsoDesc.DSVFormat             = s_DepthStencilBufferFormat;
    graphicsPsoDesc.RasterizerState       = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);    // CW front; cull back
    graphicsPsoDesc.BlendState            = CD3DX12_BLEND_DESC(D3D12_DEFAULT);         // Opaque
    graphicsPsoDesc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
    graphicsPsoDesc.SampleMask            = UINT_MAX;
    graphicsPsoDesc.SampleDesc            = DefaultSampleDesc();
    graphicsPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPsoDesc.NodeMask              = GetNodeMask();

    hr = m_DeviceHandle->CreateGraphicsPipelineState(&graphicsPsoDesc, IID_PPV_ARGS(&m_IndirectDrawPassPSO));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the graphics pipeline state");
        return false;
    }
    
    return true;
}
