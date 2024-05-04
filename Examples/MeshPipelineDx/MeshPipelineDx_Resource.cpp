#include "MeshPipelineDx.h"
#include "AssetsManager.h"
#include <random>

bool MeshPipelineDx::CreateDepthStencilBuffer()
{
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = s_DepthStencilBufferFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    m_DepthStencilBuffer = CreateTexture(s_DepthStencilBufferFormat
        , m_Width
        , m_Height
        , D3D12_RESOURCE_STATE_DEPTH_WRITE
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        , &optClear);

    if(m_DeviceHandle == nullptr)
    {
        Log::Error("[D3D12] Failed to create depth stencil buffer");
        return false;
    }

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = s_DepthStencilBufferFormat;
    dsvDesc.Texture2D.MipSlice = 0;

    m_DsvHandle = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();
    m_DeviceHandle->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &dsvDesc, m_DsvHandle);
    
    return true;
}

bool MeshPipelineDx::CreateScene()
{
    m_Camera.AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    m_Camera.Transform.SetWorldPosition(glm::vec3(0, 20, 0));
    m_Camera.Transform.LookAt(glm::vec3(0, 0, 0));
    
    m_Mesh = AssetsManager::LoadMeshImmediately("sphere.fbx");
    if(m_Mesh == nullptr || m_Mesh->IsEmpty())
        return false;

    std::vector<DirectX::CullData> cullData;
    if(!m_Mesh->ComputeMeshlets(m_Meshlets
        , m_UniqueVertexIndices
        , m_PackedPrimitiveIndices
        , cullData))
    {
        return false;
    }

    for(auto i : cullData)
    {
        m_MeshletCullData.push_back(glm::vec4(i.BoundingSphere.Center.x, i.BoundingSphere.Center.y, i.BoundingSphere.Center.z, i.BoundingSphere.Radius));
    }

    m_Mesh->GetPositionData(m_PositionData);
    m_Mesh->GetTexCoord0Data(m_TexCoord0Data);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis01(0, 1.0f);
    std::uniform_real_distribution<float> dis11(-1.0f, 1.0f);

    glm::vec3 zoom(s_InstanceCountX * 2.5f, s_InstanceCountY * 2.5f, s_InstanceCountZ * 2.5f) ;
    
    for(uint32_t x = 0; x < s_InstanceCountX; x++)
    {
        for(uint32_t y = 0; y < s_InstanceCountY; y++)
        {
            for(uint32_t z = 0; z < s_InstanceCountZ; z++)
            {
                uint32_t i = x * s_InstanceCountY * s_InstanceCountZ + y * s_InstanceCountZ + z;
                Transform transform;
                glm::vec3 pos((float)x / (float)s_InstanceCountX, (float)y / (float)s_InstanceCountY, (float)z / (float)s_InstanceCountZ);
                transform.SetWorldPosition(glm::mix(-zoom, zoom, pos));
                transform.SetLocalRotation(glm::vec3(dis11(rd), dis11(rd), dis11(rd)));
                
                m_InstancesData[i].LocalToWorld = transform.GetLocalToWorldMatrix();
                m_InstancesData[i].WorldToLocal = transform.GetWorldToLocalMatrix();
            }
        }
    }

    uint32_t totalMeshletCount = s_InstancesCount * (uint32_t)m_Meshlets.size();
    m_GroupCount = totalMeshletCount / s_ASThreadGroupSize;
    if(totalMeshletCount % s_ASThreadGroupSize != 0)
    {
        m_GroupCount += 1;;
    }

    m_MeshInfo.IndexCount = m_Mesh->GetIndicesCount();
    m_MeshInfo.VertexCount = m_Mesh->GetVerticesCount();
    m_MeshInfo.MeshletCount = (uint32_t)m_Meshlets.size();
    m_MeshInfo.InstanceCount = s_InstancesCount;

    return true;
}


bool MeshPipelineDx::CreateResources()
{
    m_CameraDataBuffer = CreateBuffer(CameraData::GetAlignedByteSizes()
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_CameraDataBuffer.Get()) return false;

    m_ViewFrustumBuffer = CreateBuffer(ViewFrustumCB::GetAlignedByteSizes()
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);
    
    if(!m_ViewFrustumBuffer.Get()) return false;

    m_MeshInfoBuffer = CreateBuffer(MeshInfo::GetAlignedByteSizes()
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);

    WriteBufferData(m_MeshInfoBuffer.Get(), &m_MeshInfo, MeshInfo::GetAlignedByteSizes());
    
    m_VerticesBuffer = CreateBuffer(m_PositionData.size() * sizeof(glm::vec4)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_VerticesBuffer.Get()) return false;

    m_TexCoordsBuffer = CreateBuffer(m_TexCoord0Data.size() * sizeof(glm::vec2)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_TexCoordsBuffer.Get()) return false;

    m_MeshletDataBuffer = CreateBuffer(m_Meshlets.size() * sizeof(DirectX::Meshlet)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_MeshletDataBuffer.Get()) return false;

    m_PackedPrimitiveIndicesBuffer = CreateBuffer(m_PackedPrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_PackedPrimitiveIndicesBuffer.Get()) return false;

    m_UniqueVertexIndicesBuffer = CreateBuffer(m_UniqueVertexIndices.size() * sizeof(uint8_t)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_UniqueVertexIndicesBuffer.Get()) return false;

    m_MeshletCullDataBuffer = CreateBuffer(m_MeshletCullData.size() * sizeof(glm::vec4)
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_MeshletCullDataBuffer.Get()) return false;

    const size_t instanceBufferBytesSize = s_InstancesCount * sizeof(InstanceData);
    m_InstanceBuffer = CreateBuffer(instanceBufferBytesSize
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);
    
    if(!m_InstanceBuffer.Get()) return false;

    BeginCommandList();
    auto stagingBuffer1 = UploadBuffer(m_VerticesBuffer.Get(), m_PositionData.data(), m_PositionData.size() * sizeof(glm::vec4));
    auto stagingBuffer2 = UploadBuffer(m_TexCoordsBuffer.Get(), m_TexCoord0Data.data(), m_TexCoord0Data.size() * sizeof(glm::vec2));
    auto stagingBuffer3 = UploadBuffer(m_MeshletDataBuffer.Get(), m_Meshlets.data(), m_Meshlets.size() * sizeof(DirectX::Meshlet));
    auto stagingBuffer4 = UploadBuffer(m_PackedPrimitiveIndicesBuffer.Get(), m_PackedPrimitiveIndices.data(), m_PackedPrimitiveIndices.size() * sizeof(DirectX::MeshletTriangle));
    auto stagingBuffer5 = UploadBuffer(m_UniqueVertexIndicesBuffer.Get(), m_UniqueVertexIndices.data(), m_UniqueVertexIndices.size() * sizeof(uint8_t));
    auto stagingBuffer6 = UploadBuffer(m_MeshletCullDataBuffer.Get(), m_MeshletCullData.data(), m_MeshletCullData.size() * sizeof(glm::vec4));
    auto stagingBuffer7 = UploadBuffer(m_InstanceBuffer.Get(), m_InstancesData.data(), instanceBufferBytesSize);

    
    std::array<CD3DX12_RESOURCE_BARRIER, 7> barriers;
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_VerticesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_TexCoordsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletDataBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_PackedPrimitiveIndicesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(m_UniqueVertexIndicesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[5] = CD3DX12_RESOURCE_BARRIER::Transition(m_MeshletCullDataBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[6] = CD3DX12_RESOURCE_BARRIER::Transition(m_InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    
    m_CommandList->ResourceBarrier((uint32_t)barriers.size(), barriers.data());
    
    EndCommandList();
    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();
    
    return true;
}

