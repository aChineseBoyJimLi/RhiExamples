#include "RayTracingPipelineDx.h"
#include <array>


bool RayTracingPipelineDx::CreateRootSignature()
{
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    
    std::array<CD3DX12_ROOT_PARAMETER, 10> globalRootParameters;
    globalRootParameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // _CameraData
    globalRootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // _LightData
    globalRootParameters[2].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_ALL); // _AccelStructure
    CD3DX12_DESCRIPTOR_RANGE descriptorRange1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    globalRootParameters[3].InitAsDescriptorTable(1, &descriptorRange1, D3D12_SHADER_VISIBILITY_ALL); // _Output
    globalRootParameters[4].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_ALL); // _Indices
    globalRootParameters[5].InitAsShaderResourceView(2, 0, D3D12_SHADER_VISIBILITY_ALL); // _Vertices
    globalRootParameters[6].InitAsShaderResourceView(3, 0, D3D12_SHADER_VISIBILITY_ALL); // _Texcoords
    globalRootParameters[7].InitAsShaderResourceView(4, 0, D3D12_SHADER_VISIBILITY_ALL); // _Normals
    globalRootParameters[8].InitAsShaderResourceView(5, 0, D3D12_SHADER_VISIBILITY_ALL); // _InstanceData
    globalRootParameters[9].InitAsShaderResourceView(6, 0, D3D12_SHADER_VISIBILITY_ALL); // _MaterialData

    CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc;
    globalRootSignatureDesc.Init((uint32_t)globalRootParameters.size()
        , globalRootParameters.data()
        , 0
        , nullptr
        , D3D12_ROOT_SIGNATURE_FLAG_NONE);

    HRESULT hr = D3D12SerializeRootSignature(&globalRootSignatureDesc
        , D3D_ROOT_SIGNATURE_VERSION_1
        , &signatureBlob
        , &errorBlob);

    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to serialize the global root signature: %s", static_cast<const char*>(errorBlob->GetBufferPointer()));
        return false;
    }

    hr = m_DeviceHandle->CreateRootSignature(GetNodeMask()
        , signatureBlob->GetBufferPointer()
        , signatureBlob->GetBufferSize()
        , IID_PPV_ARGS(&m_GlobalRootSignature));

    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the global root signature");
        return false;
    }
    
    return true;
}

bool RayTracingPipelineDx::CreateShader()
{
    m_RayGenShaderBlob = AssetsManager::LoadShaderImmediately("RayTracing.rgen.bin");
    if(!m_RayGenShaderBlob || m_RayGenShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load ray generation shader");
        return false;
    }
    
    m_MissShadersBlob = AssetsManager::LoadShaderImmediately("RayTracing.rmis.bin");
    if(!m_MissShadersBlob || m_MissShadersBlob->IsEmpty())
    {
        Log::Error("Failed to miss shader");
        return false;
    }
    
    
    m_HitGroupShadersBlob = AssetsManager::LoadShaderImmediately("RayTracing.rhit.bin");
    if(!m_HitGroupShadersBlob || m_HitGroupShadersBlob->IsEmpty())
    {
        Log::Error("Failed to hit group shaders");
        return false;
    }
    return true;
}

bool RayTracingPipelineDx::CreatePipelineState()
{
    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    auto rayGenShaderLib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE rayGenByteCode = CD3DX12_SHADER_BYTECODE(m_RayGenShaderBlob->GetData(), m_RayGenShaderBlob->GetSize());
    rayGenShaderLib->SetDXILLibrary(&rayGenByteCode);
    rayGenShaderLib->DefineExport(L"RayGen");

    auto misShaderLib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE missByteCode = CD3DX12_SHADER_BYTECODE(m_MissShadersBlob->GetData(), m_MissShadersBlob->GetSize());
    misShaderLib->SetDXILLibrary(&missByteCode);
    misShaderLib->DefineExport(L"RayMiss");
    misShaderLib->DefineExport(L"ShadowMiss");

    auto hitShaderLib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE hitByteCode = CD3DX12_SHADER_BYTECODE(m_HitGroupShadersBlob->GetData(), m_HitGroupShadersBlob->GetSize());
    hitShaderLib->SetDXILLibrary(&hitByteCode);
    hitShaderLib->DefineExport(L"ClosestHitTriangle");
    hitShaderLib->DefineExport(L"ClosestHitProceduralPrim");
    hitShaderLib->DefineExport(L"ProceduralPlanePrim");

    auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitGroup->SetClosestHitShaderImport(L"ClosestHitTriangle");
    hitGroup->SetHitGroupExport(L"HitGroup");
    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

    auto hitGroup2 = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hitGroup2->SetIntersectionShaderImport(L"ProceduralPlanePrim");
    hitGroup2->SetClosestHitShaderImport(L"ClosestHitProceduralPrim");
    hitGroup2->SetHitGroupExport(L"HitGroup2");
    hitGroup2->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);

    auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    constexpr uint32_t payloadSize =  4 * sizeof(float) + sizeof(uint32_t);
    constexpr uint32_t attributeSize = 3 * sizeof(float);
    shaderConfig->Config(payloadSize, attributeSize);

    auto globalRootSignatureSubobject = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignatureSubobject->SetRootSignature(m_GlobalRootSignature.Get());

    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    constexpr uint32_t maxRecursionDepth = s_MaxRayRecursionDepth;
    pipelineConfig->Config(maxRecursionDepth);

    HRESULT hr = m_DeviceHandle->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_PipelineState));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the pipeline state");
        return false;
    }

    hr = m_PipelineState->QueryInterface(IID_PPV_ARGS(&m_PipelineStateProperties));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }
    
    return true;
}

#define align_to(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

bool RayTracingPipelineDx::CreateShaderTable()
{
    constexpr uint32_t ragGenShaderCount = 1, missShaderCount = 2, hitGroupCount = 2;
    constexpr uint32_t shaderIdentifierCount = ragGenShaderCount + missShaderCount + hitGroupCount;
    
    constexpr size_t shaderTableStride = align_to(64, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    constexpr size_t shaderTableSize = shaderTableStride * shaderIdentifierCount;
    m_ShaderTableBuffer = CreateBuffer(shaderTableSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);

    void* pData = nullptr;
    m_ShaderTableBuffer->Map(0, nullptr, &pData);
    
    // raygen shader
    uint32_t index = 0;
    memcpy(static_cast<uint8_t*>(pData) + index * shaderTableStride
        , m_PipelineStateProperties->GetShaderIdentifier(L"RayGen")
        , shaderTableStride);
    index++;
    
    m_RayGenerationShaderRecord.StartAddress = m_ShaderTableBuffer->GetGPUVirtualAddress();
    m_RayGenerationShaderRecord.SizeInBytes = shaderTableStride;
    

    uint32_t missStartIndex = index;
    // RayMiss shader
    memcpy(static_cast<uint8_t*>(pData) + missStartIndex * shaderTableStride
        , m_PipelineStateProperties->GetShaderIdentifier(L"RayMiss")
        , shaderTableStride);
    
    index++;
    
    // ShadowMiss shader
    memcpy(static_cast<uint8_t*>(pData) + index * shaderTableStride
        , m_PipelineStateProperties->GetShaderIdentifier(L"ShadowMiss")
        , shaderTableStride);

    index++;

    m_MissShaderRecord.StartAddress = m_ShaderTableBuffer->GetGPUVirtualAddress() + shaderTableStride;
    m_MissShaderRecord.StrideInBytes = shaderTableStride;
    m_MissShaderRecord.SizeInBytes = missShaderCount * shaderTableStride;
    

    uint32_t hitGroupStartIndex = index;
    // HitGroup (Triangle Mesh)
    memcpy(static_cast<uint8_t*>(pData) + index * shaderTableStride
        , m_PipelineStateProperties->GetShaderIdentifier(L"HitGroup")
        , shaderTableStride);
    
    index++;

    // HitGroup2 (Procedural Plane)
    memcpy(static_cast<uint8_t*>(pData) + index * shaderTableStride
        , m_PipelineStateProperties->GetShaderIdentifier(L"HitGroup2")
        , shaderTableStride);

    m_HitGroupRecord.StartAddress = m_ShaderTableBuffer->GetGPUVirtualAddress() + hitGroupStartIndex * shaderTableStride;
    m_HitGroupRecord.StrideInBytes = shaderTableStride;
    m_HitGroupRecord.SizeInBytes = hitGroupCount * shaderTableStride;
    
    m_ShaderTableBuffer->Unmap(0, nullptr);
    
    return true;
}