struct VertexOutput
{
    float4 Position : SV_POSITION;
    float4 Color    : COLOR;
    float2 TexCoord : TEXCOORD0;
};

float4 main(VertexOutput input) : SV_Target
{
    return input.Color;
}