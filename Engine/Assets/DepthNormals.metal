#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct
{
    matrix_float4x4 _ModelViewProjectionMatrix;
    matrix_float4x4 _ModelViewMatrix;
} uniforms_t;

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float3 normal [[attribute(3)]];
};

typedef struct
{
    float4 position [[position]];
    float4 mvPosition;
    float4 normal;
} ColorInOut;

vertex ColorInOut depthnormals_vertex( Vertex vert [[stage_in]],
                                       constant uniforms_t& uniforms [[ buffer(5) ]],
                                       unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;
    
    float4 in_position = float4( float3( vert.position ), 1.0 );
    float4 in_normal = float4( float3( vert.normal ), 0.0 );
    out.position = uniforms._ModelViewProjectionMatrix * in_position;
    out.mvPosition = uniforms._ModelViewMatrix * in_position;
    out.normal = uniforms._ModelViewMatrix * in_normal;
    return out;
}

fragment float4 depthnormals_fragment(ColorInOut in [[stage_in]],
                              texture2d<float, access::sample> textureMap [[texture(0)]])
{
    float linearDepth = in.mvPosition.z;

    return float4( linearDepth, in.normal.xyz );
}
