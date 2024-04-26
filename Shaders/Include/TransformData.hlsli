#ifndef TRANSFORM_DATA_HLSLI
#define TRANSFORM_DATA_HLSLI

struct TransformData
{
    float4x4 LocalToWorld;
    float4x4 WorldToLocal;
};

inline float3 TransformLocalToWorld(in TransformData Data, in float3 Pos)
{
    return mul(Data.LocalToWorld, float4(Pos, 1.0f)).xyz;
}

inline float3 TransformWorldToLocal(in TransformData Data, in float3 Pos)
{
    return mul(Data.WorldToLocal, float4(Pos, 1.0f)).xyz;
}

inline float3 TransformLocalToWorldDir( in TransformData Data, in float3 Dir )
{
    return normalize(mul((float3x3)Data.LocalToWorld, Dir));
}

inline float3 TransformWorldToLocalDir( in TransformData Data, in float3 Dir )
{
    return normalize(mul((float3x3)Data.WorldToLocal, Dir));
}

// Normal need to be multiply by inverse transpose of LocalToWorld
inline float3 TransformLocalToWorldNormal( in TransformData Data, in float3 Norm )
{
    return normalize(mul(Norm, (float3x3)Data.WorldToLocal));
}

void CreateTangentToWorldBasis( in TransformData Data, in float3 Normal, in half4 Tangent, out half3 NormalWS, out half3 TangentWS, out half3 BinormalWS )
{
    NormalWS = TransformLocalToWorldNormal(Data, Normal);
    TangentWS = TransformLocalToWorldDir(Data, Tangent.xyz);
    BinormalWS = cross(NormalWS, TangentWS) * Tangent.w;
}

#endif
