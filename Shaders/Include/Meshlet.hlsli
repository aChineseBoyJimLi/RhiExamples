#ifndef MESH_LET_HLSLI
#define MESH_LET_HLSLI

#define AS_GROUP_SIZE 32
#define MS_GROUP_SIZE 128

struct MeshInfo
{
    uint VertexCount;
    uint IndexCount;
    uint MeshletCount;
    uint InstanceCount;
};

struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};

struct MeshletCullData
{
    float4 BoundingSphere; // xyz = center, w = radius
};

uint3 UnpackPrimitive(uint primitive)
{
    // Unpacks a 10 bits per index triangle from a 32-bit uint.
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

#endif // MESH_LET_HLSLI