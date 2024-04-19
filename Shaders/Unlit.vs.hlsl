#include "Include/Unlit.hlsli"

VertexOutput main(VertexInput input)
{
    VertexOutput output;
	float3 posWS = TransformLocalToWorld(_TransformData, input.position);
    float4 posHS = TransformWorldToClip(_CameraData, posWS);
    output.pos = posHS;
    output.color = input.color;
    output.texcoord = input.texcoord;
    return output;
}