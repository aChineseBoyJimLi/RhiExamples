#include "IndirectDrawDx.h"
#include <random>

static inline uint32_t AlignForUavCounter(UINT bufferSize)
{
    const UINT alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
    return (bufferSize + (alignment - 1)) & ~(alignment - 1);
}

bool IndirectDrawDx::CreateDepthStencilBuffer()
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

struct AABB
{
    glm::vec3 Min;
    float     Padding0;
    glm::vec3 Max;
    float     Padding1;
};

struct InstanceData
{
    glm::mat4 LocalToWorld;
    glm::mat4 WorldToLocal;
    AABB      AABB;
    uint32_t  MaterialIndex;
    
    // Constant buffers are 256-byte aligned. Add padding in the struct to allow multiple buffers
    // to be array-indexed.
    uint32_t Padding[23];
};

struct MaterialData
{
    glm::vec4 Color;
    float Smooth;
    uint32_t TexIndex;
    uint32_t  Padding0;
    uint32_t  Padding1;
};

struct VertexData
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

bool IndirectDrawDx::CreateResources()
{
    m_Camera.AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    m_Camera.Transform.SetWorldPosition(glm::vec3(0, 20, 20));
    m_Camera.Transform.LookAt(glm::vec3(0, 0, 0));
    
    m_Light.Transform.SetWorldForward(glm::vec3(-1,-1,-1));
    m_Light.Color = glm::vec3(1, 1, 1);
    m_Light.Intensity = 1.0f;
    
    m_Mesh = AssetsManager::LoadMeshImmediately("sphere.fbx");
    if(m_Mesh == nullptr || m_Mesh->IsEmpty()) return false;
    
    m_Textures[0] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_01_1024x1024.jpg");
    m_Textures[1] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_02_1024_1024.jpg");
    m_Textures[2] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_03_1024_1024.jpg");
    m_Textures[3] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_04_1024_1024.jpg");
    m_Textures[4] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_05_1024_1024.jpg");
    for(uint32_t i = 0; i < s_TexturesCount; ++i)
    {
        if(m_Textures[i] == nullptr || m_Textures[i]->IsEmpty())
            return false;

        const DirectX::TexMetadata& metadata = m_Textures[i]->GetTextureDesc();
        m_MainTextures[i] = CreateTexture(metadata.format
            , metadata.width
            , metadata.height
            , D3D12_RESOURCE_STATE_COPY_DEST
            , D3D12_HEAP_TYPE_DEFAULT
            , D3D12_RESOURCE_FLAG_NONE
            , nullptr);
        
        if(!m_MainTextures[i].Get())
            return false;
    }
    
    std::vector<VertexData> verticesData(m_Mesh->GetVerticesCount());
    const aiMesh* mesh = m_Mesh->GetMesh();
    for(uint32_t i = 0; i < m_Mesh->GetVerticesCount(); ++i)
    {
        verticesData[i].Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        verticesData[i].Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        verticesData[i].TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis01(0, 1.0f);
    std::uniform_real_distribution<float> dis11(-1.0f, 1.0f);
    
    constexpr uint32_t materialCount = 100;
    std::array<MaterialData, materialCount> materialsData;
    
    for(uint32_t i = 0; i < materialCount; ++i)
    {
        // random generate material data
        materialsData[i].Color = glm::vec4(1, 1, 1, 1);
        materialsData[i].Smooth = dis01(gen) * 5.0f + 0.5f;
        materialsData[i].TexIndex = static_cast<uint32_t>(s_TexturesCount * dis01(gen));
    }
    
    std::array<InstanceData, s_InstancesCount> instancesData;
    for(uint32_t i = 0; i < s_InstancesCount; ++i)
    {
        // random generate instance data
        Transform transform;
        transform.SetWorldPosition(glm::vec3(dis11(rd), dis11(rd), dis11(rd)) * 10.0f);
        float scale = dis01(rd) + 0.001f;
        transform.SetLocalScale(glm::vec3(scale, scale, scale));
        transform.SetLocalRotation(glm::vec3(dis11(rd), dis11(rd), dis11(rd)));
        instancesData[i].LocalToWorld = transform.GetLocalToWorldMatrix();
        instancesData[i].WorldToLocal = transform.GetWorldToLocalMatrix();
        instancesData[i].MaterialIndex = i % materialCount;
        instancesData[i].AABB.Min = glm::vec3(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z);
        instancesData[i].AABB.Max = glm::vec3(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z);

        ViewFrustum frustum;
        m_Camera.GetViewFrustumWorldSpace(frustum);
        for(uint32_t j = 0; j < 8; ++j)
        {
            frustum.Corners[j] = transform.WorldToLocalPoint(frustum.Corners[j]);
        }
        ViewFrustumPlanes planes = CameraBase::Corners2Planes(frustum);
        bool inter = CameraBase::IsAABBInFrustum(planes, instancesData[i].AABB.Min, instancesData[i].AABB.Max);
        const char* str = inter ? "true" : "false";
        Log::Info(str);
    }
    
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

    m_LightDataBuffer = CreateBuffer(DirectionalLightData::GetAlignedByteSizes()
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_LightDataBuffer.Get()) return false;

    const size_t vertexBufferSize = m_Mesh->GetVerticesCount() * sizeof(VertexData);
    m_VerticesBuffer = CreateBuffer(vertexBufferSize
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_VerticesBuffer.Get()) return false;

    m_IndicesBuffer = CreateBuffer(m_Mesh->GetIndicesDataByteSize()
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_IndicesBuffer.Get()) return false;
    
    const size_t instanceBufferBytesSize = s_InstancesCount * sizeof(InstanceData);
    m_InstancesBuffer = CreateBuffer(instanceBufferBytesSize
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_InstancesBuffer.Get()) return false;

    D3D12_GPU_VIRTUAL_ADDRESS instancesBufferAddress = m_InstancesBuffer->GetGPUVirtualAddress();
    std::array<IndirectCommand, s_InstancesCount> indirectCommands;
    for(uint32_t i = 0; i < s_InstancesCount; ++i)
    {
        indirectCommands[i].CBV = instancesBufferAddress + sizeof(InstanceData) * i;
        indirectCommands[i].DrawIndexedArguments.BaseVertexLocation = 0;
        indirectCommands[i].DrawIndexedArguments.InstanceCount = 1;
        indirectCommands[i].DrawIndexedArguments.StartIndexLocation = 0;
        indirectCommands[i].DrawIndexedArguments.StartInstanceLocation = 0;
        indirectCommands[i].DrawIndexedArguments.IndexCountPerInstance = m_Mesh->GetIndicesCount();
    }

    const size_t commandBufferByteSize = indirectCommands.size() * sizeof(IndirectCommand);
    m_IndirectCommandsBuffer = CreateBuffer(commandBufferByteSize
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    if(m_IndirectCommandsBuffer.Get() == nullptr) return false;

    m_CommandBufferCounterOffset = AlignForUavCounter(commandBufferByteSize);
    const size_t processedCommandsBufferBytesSize = m_CommandBufferCounterOffset + sizeof(uint32_t); 
    m_ProcessedCommandsBuffer = CreateBuffer(processedCommandsBufferBytesSize
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    if(m_ProcessedCommandsBuffer.Get() == nullptr) return false;

    m_ProcessedCommandsResetBuffer = CreateBuffer(sizeof(uint32_t)
        , D3D12_RESOURCE_STATE_GENERIC_READ
        , D3D12_HEAP_TYPE_UPLOAD
        , D3D12_RESOURCE_FLAG_NONE);
    if(m_ProcessedCommandsResetBuffer.Get() == nullptr) return false;
    
    uint8_t* pMappedCounterReset = nullptr;
    m_ProcessedCommandsResetBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pMappedCounterReset));
    ZeroMemory(pMappedCounterReset, sizeof(uint32_t)); // set count to zero
    m_ProcessedCommandsResetBuffer->Unmap(0, nullptr);

    const size_t materialsBufferBytesSize = materialCount * sizeof(MaterialData);
    m_MaterialsBuffer = CreateBuffer(materialsBufferBytesSize
        , D3D12_RESOURCE_STATE_COPY_DEST
        , D3D12_HEAP_TYPE_DEFAULT
        , D3D12_RESOURCE_FLAG_NONE);

    if(!m_MaterialsBuffer.Get()) return false;

    BeginCommandList();

    auto stagingBuffer1 = UploadBuffer(m_VerticesBuffer.Get(), verticesData.data(), vertexBufferSize);
    auto stagingBuffer2 = UploadBuffer(m_IndicesBuffer.Get(), m_Mesh->GetIndicesData(), m_Mesh->GetIndicesDataByteSize());
    auto stagingBuffer3 = UploadBuffer(m_InstancesBuffer.Get(), instancesData.data(), instanceBufferBytesSize);
    auto stagingBuffer4 = UploadBuffer(m_MaterialsBuffer.Get(), materialsData.data(), materialsBufferBytesSize);
    auto stagingBuffer5 = UploadTexture(m_MainTextures[0].Get(), m_Textures[0]->GetScratchImage());
    auto stagingBuffer6 = UploadTexture(m_MainTextures[1].Get(), m_Textures[1]->GetScratchImage());
    auto stagingBuffer7 = UploadTexture(m_MainTextures[2].Get(), m_Textures[2]->GetScratchImage());
    auto stagingBuffer8 = UploadTexture(m_MainTextures[3].Get(), m_Textures[3]->GetScratchImage());
    auto stagingBuffer9 = UploadTexture(m_MainTextures[4].Get(), m_Textures[4]->GetScratchImage());
    auto stagingBuffer10 = UploadBuffer(m_IndirectCommandsBuffer.Get(), indirectCommands.data(), commandBufferByteSize);


    std::array<CD3DX12_RESOURCE_BARRIER, 10> barriers;
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_VerticesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndicesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_InstancesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_MaterialsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(m_MainTextures[0].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[5] = CD3DX12_RESOURCE_BARRIER::Transition(m_MainTextures[1].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[6] = CD3DX12_RESOURCE_BARRIER::Transition(m_MainTextures[2].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[7] = CD3DX12_RESOURCE_BARRIER::Transition(m_MainTextures[3].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[8] = CD3DX12_RESOURCE_BARRIER::Transition(m_MainTextures[4].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[9] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndirectCommandsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_CommandList->ResourceBarrier((uint32_t)barriers.size(), barriers.data());
    
    EndCommandList();
    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();

    // Create Output commands 
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = s_InstancesCount;
    uavDesc.Buffer.StructureByteStride = sizeof(IndirectCommand);
    uavDesc.Buffer.CounterOffsetInBytes = m_CommandBufferCounterOffset;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    D3D12_CPU_DESCRIPTOR_HANDLE heapHandle = m_ShaderBoundViewHeap->GetCPUDescriptorHandleForHeapStart();
    heapHandle.ptr += m_OutputCommandsUavSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_DeviceHandle->CreateUnorderedAccessView(m_ProcessedCommandsBuffer.Get(), m_ProcessedCommandsBuffer.Get(), &uavDesc, heapHandle);
    
    // Create Texture SRVs
    for(uint32_t i = 0; i < s_TexturesCount; ++i)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC texSrv{};
        texSrv.Format = m_MainTextures[i]->GetDesc().Format;
        texSrv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        texSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        texSrv.Texture2D.MostDetailedMip = 0;
        texSrv.Texture2D.MipLevels = m_MainTextures[i]->GetDesc().MipLevels;
        texSrv.Texture2D.PlaneSlice = 0;
        texSrv.Texture2D.ResourceMinLODClamp = 0.0f;

        D3D12_CPU_DESCRIPTOR_HANDLE heapHandle = m_ShaderBoundViewHeap->GetCPUDescriptorHandleForHeapStart();
        uint32_t slot = i + m_MainTextureSrvBaseSlot;
        heapHandle.ptr += slot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_DeviceHandle->CreateShaderResourceView(m_MainTextures[i].Get(), &texSrv, heapHandle);
    }

    // Create Vertex Buffer View
    m_VertexBufferView.BufferLocation = m_VerticesBuffer->GetGPUVirtualAddress();
    m_VertexBufferView.StrideInBytes = sizeof(VertexData);
    m_VertexBufferView.SizeInBytes = vertexBufferSize;

    // Create Index Buffer View
    m_IndexBufferView.BufferLocation = m_IndicesBuffer->GetGPUVirtualAddress();
    m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_IndexBufferView.SizeInBytes = m_Mesh->GetIndicesDataByteSize();

    // Create Samplers
    D3D12_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    heapHandle = m_SamplerHeap->GetCPUDescriptorHandleForHeapStart();
    heapHandle.ptr += m_SamplerSlot * m_DeviceHandle->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_DeviceHandle->CreateSampler(&samplerDesc, heapHandle);
    
    return true;
}