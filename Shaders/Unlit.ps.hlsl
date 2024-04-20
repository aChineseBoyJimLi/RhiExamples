#include "Passes/UnlitPass.hlsl"

float4 main(VertexOutput input) : SV_Target
{
    float4 col = _MainTex.Sample(_MainTex_Sampler, input.texcoord);
    return input.color * col;
}