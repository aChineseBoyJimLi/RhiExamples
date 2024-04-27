#include "IndirectDrawDx.h"

bool IndirectDrawDx::CreateRootSignature()
{
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    
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

    m_ComputeShaderBlob = AssetsManager::LoadShaderImmediately("VisibleCulling.cs.bin");
    if(!m_ComputeShaderBlob || m_ComputeShaderBlob->IsEmpty())
    {
        Log::Error("Failed to load compute shader");
        return false;
    }
    
    return true;
}

bool IndirectDrawDx::CreatePipelineState()
{
    return true;
}
