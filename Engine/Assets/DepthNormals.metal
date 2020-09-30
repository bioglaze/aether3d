#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;
#include "MetalCommon.h"

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float3 normal [[attribute(3)]];
};

struct VertexSkin
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float3 normal [[attribute(3)]];
    int4 boneIndex [[attribute(5)]];
    float4 boneWeights [[attribute(6)]];
};

struct ColorInOut
{
    float4 position [[position]];
    float4 mvPosition;
    float4 normal;
};

vertex ColorInOut depthnormals_skin_vertex( VertexSkin vert [[stage_in]],
                                            constant Uniforms& uniforms [[ buffer(5) ]])
{
    ColorInOut out;
    
    matrix_float4x4 boneTransform = uniforms.boneMatrices[ vert.boneIndex.x ] * vert.boneWeights.x +
                                    uniforms.boneMatrices[ vert.boneIndex.y ] * vert.boneWeights.y +
                                    uniforms.boneMatrices[ vert.boneIndex.z ] * vert.boneWeights.z +
                                    uniforms.boneMatrices[ vert.boneIndex.w ] * vert.boneWeights.w;

    float4 in_position = float4( vert.position.xyz, 1.0 );
    float4 skinnedPosition = boneTransform * in_position;
    float4 in_normal = float4( vert.normal.xyz, 0.0 );
    float4 skinnedNormal = boneTransform * in_normal;
    out.position = uniforms.localToClip * skinnedPosition;
    out.mvPosition = uniforms.localToView * skinnedPosition;
    out.normal = uniforms.localToView * skinnedNormal;
    return out;
}

vertex ColorInOut depthnormals_vertex( Vertex vert [[stage_in]],
                                       constant Uniforms& uniforms [[ buffer(5) ]])
{
    ColorInOut out;
    
    float4 in_position = float4( vert.position.xyz, 1.0 );
    float4 in_normal = float4( vert.normal.xyz, 0.0 );
    out.position = uniforms.localToClip * in_position;
    out.mvPosition = uniforms.localToView * in_position;
    out.normal = uniforms.localToView * in_normal;
    return out;
}

fragment float4 depthnormals_fragment( ColorInOut in [[stage_in]] )
{
    float linearDepth = in.mvPosition.z;

    return float4( linearDepth, in.normal.xyz );
}
