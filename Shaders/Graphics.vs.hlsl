#include "Passes\GraphicsPipelinePass.hlsl"

VertexOutput main(VertexInput input, uint instanceID : SV_InstanceID)
{
    VertexOutput output;
    InstanceData instanceData = _InstanceData[instanceID];
	float3 posWS = TransformLocalToWorld(instanceData.Transform, input.Position);
    float4 posHS = TransformWorldToClip(_CameraData, posWS);
    output.Position = posHS;
    output.TexCoord = input.TexCoord;
    output.WorldNormal = TransformLocalToWorldNormal(instanceData.Transform, input.Normal);
    output.MatIndex = instanceData.MatIndex;
    return output;
}