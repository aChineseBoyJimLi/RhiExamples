#ifndef COMMON_HLSLI
#define COMMON_HLSLI

#ifdef SPIRV
#define VK_PUSH_CONSTANT [[vk::push_constant]]
#define VK_BINDING(reg,dset) [[vk::binding(reg,dset)]]
#define VK_COUNTER_BINDING(binding) [[vk::counter_binding(binding)]]
#else
#define VK_PUSH_CONSTANT
#define VK_BINDING(reg,dset)
#define VK_COUNTER_BINDING(binding)
#endif

struct IndirectDrawCommand
{
    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

struct IndexedIndirectDrawCommand
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int BaseVertexLocation;
    uint StartInstanceLocation;
#ifndef SPIRV // HLSL
    int Padding;
    int Padding1;
    int Padding2;
#endif
};

#define PI            3.14159265359f
#define INV_PI        0.31830988618f
#define EPSILON       1e-6f

#endif