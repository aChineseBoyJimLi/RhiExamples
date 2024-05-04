#include "RayTracingPipelineVk.h"
#include <random>


bool RayTracingPipelineVk::CreateScene()
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
    
    m_AABB[0] = {-1, -1, -1, 1, 1, 1}; // AABB contains a procedural geometry
    
    return true;   
}

bool RayTracingPipelineVk::CreateResources()
{
    if(!CreateTexture(m_Width
        , m_Height
        , s_BackBufferFormat
        , VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , VK_IMAGE_LAYOUT_UNDEFINED
        , m_OutputImage
        , m_OutputImageMemory))
    {
        Log::Error("Failed to create output image");
        return false;
    }

    if(!CreateImageView(m_OutputImage, s_BackBufferFormat, VK_IMAGE_ASPECT_COLOR_BIT,m_OutputImageView))
    {
        Log::Error("Failed to create output image view");
        return false;
    }
    
    if(!CreateBuffer(CameraData::GetAlignedByteSizes()
        , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        , m_CameraDataBuffer
        , m_CameraBufferMemory))
    {
        Log::Error("Failed to create camera data buffer");
        return false;
    }

    if(!CreateBuffer(DirectionalLightData::GetAlignedByteSizes()
        , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        , m_LightDataBuffer
        , m_LightDataMemory))
    {
        Log::Error("Failed to create light data buffer");
        return false;
    }

    const size_t instanceBufferBytesSize = s_InstanceCount * sizeof(InstanceData);
    if(!CreateBuffer(instanceBufferBytesSize
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_InstanceBuffer
        , m_InstanceBufferMemory))
    {
        Log::Error("Failed to create instance buffer");
        return false;
    }

    const size_t materialsBufferBytesSize = s_MaterialCount * sizeof(MaterialData);
    if(!CreateBuffer(materialsBufferBytesSize
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_MaterialsBuffer
        , m_MaterialsBufferMemory))
    {
        Log::Error("Failed to create materials buffer");
        return false;
    }

    if(!CreateBuffer(m_Mesh->GetPositionDataByteSize()
        , VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_VerticesBuffer
        , m_VerticesBufferMemory))
    {
        Log::Error("Failed to create vertex buffer");
        return false;
    }

    if(!CreateBuffer(m_Mesh->GetIndicesDataByteSize()
        , VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_IndicesBuffer
        , m_IndicesBufferMemory))
    {
        Log::Error("Failed to create index buffer");
        return false;
    }

    if(!CreateBuffer(m_TexCoord0Data.size() * sizeof(glm::vec2)
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_TexcoordsBuffer
        , m_TexcoordsBufferMemory))
    {
        Log::Error("Failed to create index buffer");
        return false;
    }

    if(!CreateBuffer(m_NormalData.size() * sizeof(glm::vec4)
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_NormalsBuffer
        , m_NormalsBufferMemory))
    {
        Log::Error("Failed to create index buffer");
        return false;
    }

    if(!CreateBuffer(sizeof(AABB) * s_AABBMeshInstanceCount
        , VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_AABBBuffer
        , m_AABBBufferMemory))
    {
        Log::Error("Failed to create AABB buffer");
        return false;
    }
    

    std::vector<std::shared_ptr<StagingBuffer>> stagingBuffers(7);

    BeginCommandList();

    stagingBuffers[0] = UploadBuffer(m_VerticesBuffer, m_Mesh->GetPositionData(), m_Mesh->GetPositionDataByteSize());
    stagingBuffers[1] = UploadBuffer(m_IndicesBuffer, m_Mesh->GetIndicesData(), m_Mesh->GetIndicesDataByteSize());
    stagingBuffers[2] = UploadBuffer(m_InstanceBuffer, m_InstancesData.data(), instanceBufferBytesSize);
    stagingBuffers[3] = UploadBuffer(m_MaterialsBuffer, m_MaterialsData.data(), materialsBufferBytesSize);
    stagingBuffers[4] = UploadBuffer(m_TexcoordsBuffer, m_TexCoord0Data.data(), m_TexCoord0Data.size() * sizeof(glm::vec2));
    stagingBuffers[5] = UploadBuffer(m_NormalsBuffer, m_NormalData.data(), m_NormalData.size() * sizeof(glm::vec4));
    stagingBuffers[6] = UploadBuffer(m_AABBBuffer, m_AABB.data(), m_AABB.size() * sizeof(AABB));

    VkImageMemoryBarrier imageBarrier  = ImageMemoryBarrier(m_OutputImage
        , VK_IMAGE_LAYOUT_UNDEFINED
        , VK_IMAGE_LAYOUT_GENERAL
        , VK_ACCESS_NONE
        , VK_ACCESS_TRANSFER_WRITE_BIT
        , VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdPipelineBarrier(m_CmdBufferHandle
        , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
        , VK_PIPELINE_STAGE_TRANSFER_BIT
        , 0
        , 0
        , nullptr
        , 0
        , nullptr
        , 1
        , &imageBarrier);
    
    EndCommandList();

    ExecuteCommandBuffer();
    
    return true;
}

void RayTracingPipelineVk::DestroyResources()
{
    if(m_AABBBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_AABBBufferMemory, nullptr);
        m_AABBBufferMemory = VK_NULL_HANDLE;
    }
    if(m_AABBBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_AABBBuffer, nullptr);
        m_AABBBuffer = VK_NULL_HANDLE;
    }
    if(m_NormalsBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_NormalsBufferMemory, nullptr);
        m_NormalsBufferMemory = VK_NULL_HANDLE;
    }
    if(m_NormalsBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_NormalsBuffer, nullptr);
        m_NormalsBuffer = VK_NULL_HANDLE;
    }
    if(m_TexcoordsBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_TexcoordsBufferMemory, nullptr);
        m_TexcoordsBufferMemory = VK_NULL_HANDLE;
    }
    if(m_TexcoordsBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_TexcoordsBuffer, nullptr);
        m_TexcoordsBuffer = VK_NULL_HANDLE;
    }
    if(m_MaterialsBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_MaterialsBufferMemory, nullptr);
        m_MaterialsBufferMemory = VK_NULL_HANDLE;
    }
    if(m_MaterialsBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_MaterialsBuffer, nullptr);
        m_MaterialsBuffer = VK_NULL_HANDLE;
    }
    if(m_InstanceBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_InstanceBufferMemory, nullptr);
        m_InstanceBufferMemory = VK_NULL_HANDLE;
    }
    if(m_InstanceBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_InstanceBuffer, nullptr);
        m_InstanceBuffer = VK_NULL_HANDLE;
    }
    if(m_IndicesBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_IndicesBufferMemory, nullptr);
        m_IndicesBufferMemory = VK_NULL_HANDLE;
    }
    if(m_IndicesBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_IndicesBuffer, nullptr);
        m_IndicesBuffer = VK_NULL_HANDLE;
    }
    if(m_VerticesBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_VerticesBufferMemory, nullptr);
        m_VerticesBufferMemory = VK_NULL_HANDLE;
    }
    if(m_VerticesBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_VerticesBuffer, nullptr);
        m_VerticesBuffer = VK_NULL_HANDLE;
    }
    if(m_LightDataMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_LightDataMemory, nullptr);
        m_LightDataMemory = VK_NULL_HANDLE;
    }
    if(m_LightDataBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_LightDataBuffer, nullptr);
        m_LightDataBuffer = VK_NULL_HANDLE;
    }
    if(m_CameraBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_CameraBufferMemory, nullptr);
        m_CameraBufferMemory = VK_NULL_HANDLE;
    }
    if(m_CameraDataBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_CameraDataBuffer, nullptr);
        m_CameraDataBuffer = VK_NULL_HANDLE;
    }

    if(m_OutputImageMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_OutputImageMemory, nullptr);
        m_OutputImageMemory = VK_NULL_HANDLE;
    }

    if(m_OutputImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(m_DeviceHandle, m_OutputImage, nullptr);
        m_OutputImage = VK_NULL_HANDLE;
    }

    if(m_OutputImageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_DeviceHandle, m_OutputImageView, nullptr);
        m_OutputImageView = VK_NULL_HANDLE;
    }
}

bool RayTracingPipelineVk::CreateBottomLevelAccelStructure()
{
    
    // Create triangle mesh BLAS
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = GetBufferDeviceAddress(m_VerticesBuffer);
    accelerationStructureGeometry.geometry.triangles.maxVertex = m_Mesh->GetVerticesCount();
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(aiVector3D);
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = GetBufferDeviceAddress(m_IndicesBuffer);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    
    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    const uint32_t numTriangleMesh = 1;
	
    vkGetAccelerationStructureBuildSizesKHR(
        m_DeviceHandle,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &numTriangleMesh,
        &accelerationStructureBuildSizesInfo);

    if(!CreateBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize
        , VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_BLASBuffer
        , m_BLASBufferMemory))
    {
        Log::Error("Failed to create BLAS buffer");
        return false;
    }
    
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = m_BLASBuffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    VkResult result = vkCreateAccelerationStructureKHR(m_DeviceHandle, &accelerationStructureCreateInfo, nullptr, &m_BLAS);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create BLAS");
        return false;
    }

    VkBuffer scratchBuffer;
    VkDeviceMemory scratchBufferMemory;

    if(!CreateBuffer(accelerationStructureBuildSizesInfo.buildScratchSize
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , scratchBuffer
        , scratchBufferMemory))
    {
        Log::Error("Failed to create BLAS scratch buffer");
        return false;
    }

    // Create procedural geometry AABB BLAS
    VkAccelerationStructureGeometryKHR accelerationStructureGeometryAABB{};
    accelerationStructureGeometryAABB.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometryAABB.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometryAABB.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    accelerationStructureGeometryAABB.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    accelerationStructureGeometryAABB.geometry.aabbs.data.deviceAddress = GetBufferDeviceAddress(m_AABBBuffer);
    accelerationStructureGeometryAABB.geometry.aabbs.stride = sizeof(AABB);

    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometryAABB;

    const uint32_t numAABB = 1;
    vkGetAccelerationStructureBuildSizesKHR(
        m_DeviceHandle,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &numAABB,
        &accelerationStructureBuildSizesInfo);

    if(!CreateBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize
        , VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_ProceduralGeoBLASBuffer
        , m_ProceduralGeoBLASBufferMemory))
    {
        Log::Error("Failed to create procedural geometry BLAS buffer");
        return false;
    }
    
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = m_ProceduralGeoBLASBuffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    
    result = vkCreateAccelerationStructureKHR(m_DeviceHandle, &accelerationStructureCreateInfo, nullptr, &m_ProceduralGeoBLAS);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create procedural geometry BLAS");
        return false;
    }

    VkBuffer proceduralGeoScratchBuffer;
    VkDeviceMemory proceduralGeoScratchBufferMemory;
    
    if(!CreateBuffer(accelerationStructureBuildSizesInfo.buildScratchSize
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , proceduralGeoScratchBuffer
        , proceduralGeoScratchBufferMemory))
    {
        Log::Error("Failed to create procedural geometry BLAS scratch buffer");
        return false;
    }
    
    // Build BLAS
    BeginCommandList();

    BuildAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
        , m_BLAS
        , scratchBuffer
        , &accelerationStructureGeometry
        , m_Mesh->GetPrimCount());

    BuildAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
        , m_ProceduralGeoBLAS
        , proceduralGeoScratchBuffer
        , &accelerationStructureGeometryAABB
        , numAABB);
    
    EndCommandList();

    ExecuteCommandBuffer();

    vkFreeMemory(m_DeviceHandle, proceduralGeoScratchBufferMemory, nullptr);
    vkDestroyBuffer(m_DeviceHandle, proceduralGeoScratchBuffer, nullptr);
    vkFreeMemory(m_DeviceHandle, scratchBufferMemory, nullptr);
    vkDestroyBuffer(m_DeviceHandle, scratchBuffer, nullptr);
    
    return true;
}

void RayTracingPipelineVk::DestroyBottomLevelAccelStructure()
{
    vkDestroyAccelerationStructureKHR(m_DeviceHandle, m_ProceduralGeoBLAS, nullptr);
    vkFreeMemory(m_DeviceHandle, m_ProceduralGeoBLASBufferMemory, nullptr);
    vkDestroyBuffer(m_DeviceHandle, m_ProceduralGeoBLASBuffer, nullptr);
    
    vkDestroyAccelerationStructureKHR(m_DeviceHandle, m_BLAS, nullptr);
    vkFreeMemory(m_DeviceHandle, m_BLASBufferMemory, nullptr);
    vkDestroyBuffer(m_DeviceHandle, m_BLASBuffer, nullptr);
}

bool RayTracingPipelineVk::CreateTopLevelAccelStructure()
{
    std::array<VkAccelerationStructureInstanceKHR, s_InstanceCount> instanceDescs {};

    // Create TLAS
    for(uint32_t i = 0; i < s_TriangleMeshInstanceCount; ++i)
    {
        instanceDescs[i].instanceCustomIndex = i;
        instanceDescs[i].mask = 0xFF;
        instanceDescs[i].instanceShaderBindingTableRecordOffset = 0; // HitGroup 0
        instanceDescs[i].flags = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
        instanceDescs[i].accelerationStructureReference = GetAccelerationStructureDeviceAddress(m_BLAS);
        m_InstancesTransform[i].GetLocalToWorld3x4(instanceDescs[i].transform.matrix);
    }

    // Procedural geometry instance
    instanceDescs[s_TriangleMeshInstanceCount].instanceCustomIndex = s_TriangleMeshInstanceCount;
    instanceDescs[s_TriangleMeshInstanceCount].mask = 0xFF;
    instanceDescs[s_TriangleMeshInstanceCount].instanceShaderBindingTableRecordOffset = 1; // HitGroup 1
    instanceDescs[s_TriangleMeshInstanceCount].flags = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
    instanceDescs[s_TriangleMeshInstanceCount].accelerationStructureReference = GetAccelerationStructureDeviceAddress(m_ProceduralGeoBLAS);
    m_InstancesTransform[s_TriangleMeshInstanceCount].GetLocalToWorld3x4(instanceDescs[s_TriangleMeshInstanceCount].transform.matrix);

    uint64_t instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instanceDescs.size();
    VkBuffer instanceBuffer;
    VkDeviceMemory instanceBufferMemory;
    if(!CreateBuffer(instanceBufferSize
        , VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        , instanceBuffer
        , instanceBufferMemory))
    {
        Log::Error("Failed to create instance buffer");
        return false;
    }

    WriteBufferData(m_DeviceHandle, instanceBufferMemory, instanceDescs.data(), instanceBufferSize);
    
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data.deviceAddress = GetBufferDeviceAddress(instanceBuffer);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    uint32_t numPrimitive = 1;
    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        m_DeviceHandle,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &numPrimitive,
        &accelerationStructureBuildSizesInfo);

    if(!CreateBuffer(accelerationStructureBuildSizesInfo.accelerationStructureSize
        , VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , m_TLASBuffer
        , m_TLASBufferMemory))
    {
        Log::Error("Failed to create TLAS buffer");
        return false;
    }
    
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = m_TLASBuffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    VkResult result = vkCreateAccelerationStructureKHR(m_DeviceHandle, &accelerationStructureCreateInfo, nullptr, &m_TLAS);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create TLAS");
        return false;
    }

    VkBuffer scratchBuffer;
    VkDeviceMemory scratchBufferMemory;

    if(!CreateBuffer(accelerationStructureBuildSizesInfo.buildScratchSize
        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        , scratchBuffer
        , scratchBufferMemory))
    {
        Log::Error("Failed to create TLAS scratch buffer");
        return false;
    }

    // Build TLAS
    
    BeginCommandList();
    
    BuildAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
        , m_TLAS
        , scratchBuffer
        , &accelerationStructureGeometry
        , s_InstanceCount);
   
    EndCommandList();
    ExecuteCommandBuffer();

    vkFreeMemory(m_DeviceHandle, scratchBufferMemory, nullptr);
    vkDestroyBuffer(m_DeviceHandle, scratchBuffer, nullptr);

    vkFreeMemory(m_DeviceHandle, instanceBufferMemory, nullptr);
    vkDestroyBuffer(m_DeviceHandle, instanceBuffer, nullptr);
    
    return true;
}

void RayTracingPipelineVk::DestroyTopLevelAccelStructure()
{
    if(m_TLAS != VK_NULL_HANDLE)
    {
        vkDestroyAccelerationStructureKHR(m_DeviceHandle, m_TLAS, nullptr);
        m_TLAS = VK_NULL_HANDLE;
    }
    
    if(m_TLASBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_DeviceHandle, m_TLASBufferMemory, nullptr);
        m_TLASBufferMemory = VK_NULL_HANDLE;
    }

    if(m_TLASBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(m_DeviceHandle, m_TLASBuffer, nullptr);
        m_TLASBuffer = VK_NULL_HANDLE;
    }
}

bool RayTracingPipelineVk::CreateDescriptorSet()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo {};
    descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocInfo.descriptorPool = m_DescriptorPoolHandle;
    descriptorSetAllocInfo.pSetLayouts = &m_DescriptorLayout;
    descriptorSetAllocInfo.descriptorSetCount = 1;
    if(vkAllocateDescriptorSets(m_DeviceHandle, &descriptorSetAllocInfo, &m_DescriptorSet) != VK_SUCCESS)
    {
        Log::Error("Failed to allocate descriptor set");
        return false;
    }
    
    std::array<VkWriteDescriptorSet, 9> descriptorWrites{};
    VkDescriptorBufferInfo cameraBufferInfo = CreateDescriptorBufferInfo(m_CameraDataBuffer, CameraData::GetAlignedByteSizes());
    UpdateBufferDescriptor(descriptorWrites[0], m_DescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &cameraBufferInfo, GetBindingSlot(ERegisterType::ConstantBuffer, 0)); // _CameraData 
    
    VkDescriptorBufferInfo lightBufferInfo = CreateDescriptorBufferInfo(m_LightDataBuffer, DirectionalLightData::GetAlignedByteSizes());
    UpdateBufferDescriptor(descriptorWrites[1], m_DescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &lightBufferInfo, GetBindingSlot(ERegisterType::ConstantBuffer, 1)); // _LightData

    VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
    descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures = &m_TLAS;
    descriptorAccelerationStructureInfo.pNext = nullptr;

    descriptorWrites[2].pNext = &descriptorAccelerationStructureInfo;
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = m_DescriptorSet;
    descriptorWrites[2].dstBinding = GetBindingSlot(ERegisterType::ShaderResource, 0); // _AccelStructure
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;


    VkDescriptorImageInfo imageInfo = CreateDescriptorImageInfo(m_OutputImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL);
    UpdateImageDescriptor(descriptorWrites[3], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageInfo, 1, GetBindingSlot(ERegisterType::UnorderedAccess, 0)); // _OutputImage
    

    VkDescriptorBufferInfo indexBufferInfo = CreateDescriptorBufferInfo(m_IndicesBuffer, m_Mesh->GetIndicesDataByteSize());
    UpdateBufferDescriptor(descriptorWrites[4], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &indexBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 1)); // _Indices

    VkDescriptorBufferInfo texcoordBufferInfo = CreateDescriptorBufferInfo(m_TexcoordsBuffer, m_TexCoord0Data.size() * sizeof(glm::vec2));
    UpdateBufferDescriptor(descriptorWrites[5], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &texcoordBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 2)); // _Texcoords

    VkDescriptorBufferInfo normalBufferInfo = CreateDescriptorBufferInfo(m_NormalsBuffer, m_NormalData.size() * sizeof(glm::vec4));
    UpdateBufferDescriptor(descriptorWrites[6], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &normalBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 3)); // _Normals
    
    const size_t instanceBufferBytesSize = s_InstanceCount * sizeof(InstanceData);
    VkDescriptorBufferInfo instanceBufferInfo = CreateDescriptorBufferInfo(m_InstanceBuffer, instanceBufferBytesSize);
    UpdateBufferDescriptor(descriptorWrites[7], m_DescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &instanceBufferInfo, GetBindingSlot(ERegisterType::ShaderResource, 4)); // _InstanceData


    const size_t materialsBufferBytesSize = s_MaterialCount * sizeof(MaterialData);
    VkDescriptorBufferInfo materialsBufferInfo = CreateDescriptorBufferInfo(m_MaterialsBuffer, materialsBufferBytesSize);
    UpdateBufferDescriptor(descriptorWrites[8] , m_DescriptorSet , VK_DESCRIPTOR_TYPE_STORAGE_BUFFER , &materialsBufferInfo , GetBindingSlot(ERegisterType::ShaderResource, 5)); // _MaterialData
    
    vkUpdateDescriptorSets(m_DeviceHandle, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

    return true;
}

void RayTracingPipelineVk::DestroyDescriptorSet()
{
    if(m_DescriptorSet != VK_NULL_HANDLE)
    {
        vkFreeDescriptorSets(m_DeviceHandle, m_DescriptorPoolHandle, 1, &m_DescriptorSet);
        m_DescriptorSet = VK_NULL_HANDLE;
    }
}