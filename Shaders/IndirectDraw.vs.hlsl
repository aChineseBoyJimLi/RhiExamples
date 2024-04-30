#include "Passes/IndirectDrawPass.hlsl"

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    float3 posWS = TransformLocalToWorld(_InstanceData.Transform, input.Position);
    float4 posHS = TransformWorldToClip(_CameraData, posWS);
    output.Position = posHS;
    output.TexCoord = input.TexCoord;
    output.WorldNormal = TransformLocalToWorldNormal(_InstanceData.Transform, input.Normal);
    output.MatIndex = _InstanceData.MatIndex;
    return output;
}