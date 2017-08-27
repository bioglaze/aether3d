#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    matrix_float4x4 localToClip;
    matrix_float4x4 localToView;
};

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float3 normal [[attribute(3)]];
};

struct ColorInOut
{
    float4 position [[position]];
    float4 mvPosition;
    float4 normal;
};

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
