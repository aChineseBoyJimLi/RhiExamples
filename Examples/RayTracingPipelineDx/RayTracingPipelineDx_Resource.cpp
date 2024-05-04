#include "RayTracingPipelineDx.h"

bool RayTracingPipelineDx::CreateScene()
{
    m_MainLight.Transform.SetWorldForward(glm::vec3(-1, -1, -1));
    m_MainLight.Color = {1.0f, 1.0f, 1.0f};
    m_MainLight.Intensity = 1.0f;
    
    m_Camera.AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    m_Camera.Transform.SetWorldPosition(glm::vec3(0, 5, 20));
    m_Camera.Transform.LookAt(glm::vec3(0, 0, 0));

    m_Mesh = AssetsManager::LoadMeshImmediately("sphere.fbx");
    
    for(uint32_t i = 0; i < m_Mesh->GetVerticesCount(); ++i)
    {
        m_Mesh->GetTexCoord0Data(m_TexCoord0Data);
        m_Mesh->GetNormalData(m_NormalData);
    }
    
    m_InstancesTransform[0].SetWorldPosition(glm::vec3(-2, 0, -2));
    m_InstancesTransform[0].SetLocalScale(glm::vec3(3));
    m_InstancesTransform[1].SetWorldPosition(glm::vec3(1, 1, 2));
    
    m_InstancesTransform[2].SetWorldPosition(glm::vec3(0,0,0)); // AABB transform
    m_InstancesTransform[2].SetLocalScale(glm::vec3(10));

    m_MaterialsData[0].Color = glm::vec4(1, 1, 1, 1);
    m_MaterialsData[0].TextureIndex = 0;
    m_MaterialsData[0].IsLambertian = 0; // Mirror reflection

    m_MaterialsData[1].Color = glm::vec4(0.2524475f, 0.8773585f, 0.4443824f, 1);
    m_MaterialsData[1].TextureIndex = 0;
    m_MaterialsData[1].IsLambertian = 1; // Lambertian

    m_InstancesData[0].LocalToWorld = m_InstancesTransform[0].GetLocalToWorldMatrix();
    m_InstancesData[0].WorldToLocal = m_InstancesTransform[0].GetWorldToLocalMatrix();
    m_InstancesData[0].MaterialIndex = 1;

    m_InstancesData[1].LocalToWorld = m_InstancesTransform[1].GetLocalToWorldMatrix();
    m_InstancesData[1].WorldToLocal = m_InstancesTransform[1].GetWorldToLocalMatrix();
    m_InstancesData[1].MaterialIndex = 1;

    m_InstancesData[2].LocalToWorld = m_InstancesTransform[2].GetLocalToWorldMatrix();
    m_InstancesData[2].WorldToLocal = m_InstancesTransform[2].GetWorldToLocalMatrix();
    m_InstancesData[2].MaterialIndex = 0;
    
    if(m_Mesh == nullptr || m_Mesh->GetMesh() == nullptr)
    {
        Log::Error("Failed to load mesh");
        return false;
    }

    m_Texture = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_01_1024x1024.jpg");

    m_AABB[0] = {-1, -1, -1, 1, 1, 1}; // AABB contains a procedural geometry
    
    return true;   
}

bool RayTracingPipelineDx::CreateResource()
{
    m_CameraDataBuffer = CreateBuffer(CameraData::GetAlignedByteSizes(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
    if(m_CameraDataBuffer.Get() == nullptr) return false;
    
    m_LightDataBuffer = CreateBuffer(DirectionalLightData::GetAlignedByteSizes(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
    if(m_LightDataBuffer.Get() == nullptr) return false;
    
    m_OutputBuffer = CreateTexture(DXGI_FORMAT_R8G8B8A8_UNORM, m_Width, m_Height, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr);
    if(m_OutputBuffer.Get() == nullptr) return false;
    
    m_VerticesBuffer = CreateBuffer(m_Mesh->GetPositionDataByteSize(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_VerticesBuffer.Get() == nullptr) return false;
    
    m_IndicesBuffer = CreateBuffer(m_Mesh->GetIndicesDataByteSize(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_IndicesBuffer.Get() == nullptr) return false;

    m_InstanceBuffer = CreateBuffer(sizeof(InstanceData) * s_InstanceCount, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_InstanceBuffer.Get() == nullptr) return false;

    m_MaterialBuffer = CreateBuffer(sizeof(MaterialData) * s_MaterialCount, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_MaterialBuffer.Get() == nullptr) return false;
    
    m_TexcoordsBuffer = CreateBuffer(m_TexCoord0Data.size() * sizeof(glm::vec2), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_TexcoordsBuffer.Get() == nullptr) return false;
    
    m_NormalsBuffer = CreateBuffer(m_NormalData.size() * sizeof(glm::vec4), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);
    if(m_NormalsBuffer.Get() == nullptr) return false;

    m_AABBBuffer = CreateBuffer(sizeof(D3D12_RAYTRACING_AABB) * s_AABBMeshInstanceCount, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
    if(m_AABBBuffer.Get() == nullptr) return false;
    WriteBufferData(m_AABBBuffer.Get(), m_AABB.data(), sizeof(D3D12_RAYTRACING_AABB) * s_AABBMeshInstanceCount);

    BeginCommandList();

    auto stagingBuffer1 = UploadBuffer(m_VerticesBuffer.Get(), m_Mesh->GetPositionData(), m_Mesh->GetPositionDataByteSize());
    auto stagingBuffer2 = UploadBuffer(m_IndicesBuffer.Get(), m_Mesh->GetIndicesData(), m_Mesh->GetIndicesDataByteSize());
    auto stagingBuffer3 = UploadBuffer(m_TexcoordsBuffer.Get(), m_TexCoord0Data.data(), m_TexCoord0Data.size() * sizeof(glm::vec2));
    auto stagingBuffer4 = UploadBuffer(m_NormalsBuffer.Get(), m_NormalData.data(), m_NormalData.size() * sizeof(glm::vec4));
    auto stagingBuffer5 = UploadBuffer(m_InstanceBuffer.Get(), m_InstancesData.data(), sizeof(InstanceData) * s_InstanceCount);
    auto stagingBuffer6 = UploadBuffer(m_MaterialBuffer.Get(), m_MaterialsData.data(), sizeof(MaterialData) * s_MaterialCount);
    
    std::array<CD3DX12_RESOURCE_BARRIER, 6> barriers;
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_VerticesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_IndicesBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_TexcoordsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_NormalsBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(m_InstanceBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    barriers[5] = CD3DX12_RESOURCE_BARRIER::Transition(m_MaterialBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    m_CommandList->ResourceBarrier((uint32_t)barriers.size(), barriers.data());
    
    EndCommandList();
    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();

    D3D12_UNORDERED_ACCESS_VIEW_DESC outputUavDesc{};
    outputUavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    outputUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    outputUavDesc.Texture2D.MipSlice = 0;
    outputUavDesc.Texture2D.PlaneSlice = 0;
    outputUavDesc.Texture2DArray.ArraySize = 1;
    outputUavDesc.Texture2DArray.FirstArraySlice = 0;
    m_DeviceHandle->CreateUnorderedAccessView(m_OutputBuffer.Get(), nullptr, &outputUavDesc, m_ShaderBoundViewHeap->GetCPUDescriptorHandleForHeapStart());
    
    return true;
}

bool RayTracingPipelineDx::CreateBottomLevelAccelStructure()
{
    // Triangle mesh geometry blas -------------------------------------
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_VerticesBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(aiVector3D);
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = m_Mesh->GetVerticesCount();
    geometryDesc.Triangles.IndexBuffer = m_IndicesBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = m_Mesh->GetIndicesCount();
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;

    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
    
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs{};
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;
    bottomLevelInputs.NumDescs = 1;
    bottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    m_DeviceHandle->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    if(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes == 0)
    {
        Log::Error("Failed to get triangle mesh geometry bottom level prebuild info");
        return false;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer = CreateBuffer(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_BLASBuffer = CreateBuffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    if(m_BLASBuffer.Get() == nullptr) return false;


    // Procedural geometry blas -------------------------------------
    D3D12_RAYTRACING_GEOMETRY_DESC proceduralGeomertryDesc{};
    proceduralGeomertryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
    proceduralGeomertryDesc.AABBs.AABBCount = 1;
    proceduralGeomertryDesc.AABBs.AABBs.StartAddress = m_AABBBuffer->GetGPUVirtualAddress();
    proceduralGeomertryDesc.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
    proceduralGeomertryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS proceduralGeomertryBottomLevelInputs{};
    proceduralGeomertryBottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    proceduralGeomertryBottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    proceduralGeomertryBottomLevelInputs.pGeometryDescs = &proceduralGeomertryDesc;
    proceduralGeomertryBottomLevelInputs.NumDescs = 1;
    proceduralGeomertryBottomLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    m_DeviceHandle->GetRaytracingAccelerationStructurePrebuildInfo(&proceduralGeomertryBottomLevelInputs, &bottomLevelPrebuildInfo);
    if(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes == 0)
    {
        Log::Error("Failed to get procedural geometry bottom level prebuild info");
        return false;
    }
    
    Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer2 = CreateBuffer(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_ProceduralGeoBLASBuffer = CreateBuffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    if(m_ProceduralGeoBLASBuffer.Get() == nullptr) return false;
    

    // Build BLAS --------------------------------------------------
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc{};
    bottomLevelBuildDesc.Inputs = bottomLevelInputs;
    bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
    bottomLevelBuildDesc.DestAccelerationStructureData = m_BLASBuffer->GetGPUVirtualAddress();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc2{};
    bottomLevelBuildDesc2.Inputs = proceduralGeomertryBottomLevelInputs;
    bottomLevelBuildDesc2.ScratchAccelerationStructureData = scratchBuffer2->GetGPUVirtualAddress();
    bottomLevelBuildDesc2.DestAccelerationStructureData = m_ProceduralGeoBLASBuffer->GetGPUVirtualAddress();

    BeginCommandList();
    m_CommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
    m_CommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc2, 0, nullptr);
    CD3DX12_RESOURCE_BARRIER barrier[2];
    barrier[0] = CD3DX12_RESOURCE_BARRIER::UAV(m_BLASBuffer.Get());
    barrier[1] = CD3DX12_RESOURCE_BARRIER::UAV(m_ProceduralGeoBLASBuffer.Get());
    m_CommandList->ResourceBarrier(2, barrier);
    EndCommandList();

    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();
    
    return true;
}

bool RayTracingPipelineDx::CreateTopLevelAccelStructure()
{
    std::array<D3D12_RAYTRACING_INSTANCE_DESC, s_InstanceCount> instanceDescs {};

    // Triangle mesh instances
    for(uint32_t i = 0; i < s_TriangleMeshInstanceCount; ++i)
    {
        instanceDescs[i].InstanceID = i;
        instanceDescs[i].InstanceContributionToHitGroupIndex = 0; // HitGroup
        instanceDescs[i].InstanceMask = 0xFF;
        instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
        instanceDescs[i].AccelerationStructure = m_BLASBuffer->GetGPUVirtualAddress();
        m_InstancesTransform[i].GetLocalToWorld3x4(instanceDescs[i].Transform);
    }

    // Procedural geometry instance
    instanceDescs[s_TriangleMeshInstanceCount].InstanceID = s_TriangleMeshInstanceCount;
    instanceDescs[s_TriangleMeshInstanceCount].InstanceContributionToHitGroupIndex = 1; // HitGroup2
    instanceDescs[s_TriangleMeshInstanceCount].InstanceMask = 0xFF;
    instanceDescs[s_TriangleMeshInstanceCount].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDescs[s_TriangleMeshInstanceCount].AccelerationStructure = m_ProceduralGeoBLASBuffer->GetGPUVirtualAddress();
    m_InstancesTransform[s_TriangleMeshInstanceCount].GetLocalToWorld3x4(instanceDescs[s_TriangleMeshInstanceCount].Transform);
    

    uint64_t instanceBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceDescs.size();
    Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer = CreateBuffer(instanceBufferSize, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_FLAG_NONE);
    WriteBufferData(instanceBuffer.Get(), instanceDescs.data(), instanceBufferSize);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs{};
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.InstanceDescs = instanceBuffer->GetGPUVirtualAddress();
    topLevelInputs.NumDescs = (uint32_t)instanceDescs.size();
    topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    m_DeviceHandle->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    if(topLevelPrebuildInfo.ResultDataMaxSizeInBytes == 0)
    {
        Log::Error("Failed to get top level prebuild info");
        return false;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer = CreateBuffer(topLevelPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    m_TLASBuffer = CreateBuffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc{};
    topLevelBuildDesc.Inputs = topLevelInputs;
    topLevelBuildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
    topLevelBuildDesc.DestAccelerationStructureData = m_TLASBuffer->GetGPUVirtualAddress();

    BeginCommandList();
    m_CommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    EndCommandList();
    ID3D12CommandList* commandLists[] = {m_CommandList.Get()};
    m_CommandQueueHandle->ExecuteCommandLists(1, commandLists);
    FlushCommandQueue();
    
    return true;
}


