#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct
{
    matrix_float4x4 _ModelViewProjectionMatrix;
    matrix_float4x4 _ModelViewMatrix;
} uniforms_t;

typedef struct
{
    packed_float3 position;
    packed_float2 texcoord;
    packed_float3 normal;
    packed_float4 tangent;
    packed_float4 color;
} vertex_t;

typedef struct
{
    float4 position [[position]];
    float4 mvPosition;
    float4 normal;
} ColorInOut;

vertex ColorInOut depthnormals_vertex(device vertex_t* vertex_array [[ buffer(0) ]],
                               constant uniforms_t& uniforms [[ buffer(1) ]],
                               unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;
    
    float4 in_position = float4( float3( vertex_array[ vid ].position ), 1.0 );
    float4 in_normal = float4( float3( vertex_array[ vid ].normal ), 0.0 );
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
