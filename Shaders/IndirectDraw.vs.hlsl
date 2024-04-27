#include "Passes/IndirectDrawPass.hlsl"

VertexOutput main(VertexInput input, uint instanceID : SV_InstanceID)
{
    VertexOutput output;

    output.Position = float4(input.Position, 1);

    return output;
}