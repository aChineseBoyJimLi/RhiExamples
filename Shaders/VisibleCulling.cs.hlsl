#include "Passes/IndirectDrawPass.hlsl"

ConstantBuffer<CameraData>      _CameraData : register(b0);
ConstantBuffer<ViewFrustum>     _ViewFrustum : register(b1);
StructuredBuffer<InstanceData>  _InstanceData : register(t0);

[numthreads(1, 1, 1)]
void main()
{
    
}