#include "Passes/MeshletViewerPass.hlsl"

float4 main(VertexOutput input) : SV_Target
{
    float4 col = _MainTex.Sample(_MainTex_Sampler, input.TexCoord);
    return input.Color * col;
}