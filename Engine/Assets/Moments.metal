#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct
{
    matrix_float4x4 _ModelViewProjectionMatrix;
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
} ColorInOut;

vertex ColorInOut moments_vertex(device vertex_t* vertex_array [[ buffer(0) ]],
                               constant uniforms_t& uniforms [[ buffer(1) ]],
                               unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;
    
    float4 in_position = float4( float3( vertex_array[ vid ].position ), 1.0 );
    out.position = uniforms._ModelViewProjectionMatrix * in_position;
    
    return out;
}

fragment half4 moments_fragment(ColorInOut in [[stage_in]],
                              texture2d<float, access::sample> textureMap [[texture(0)]])
{
    float linearDepth = in.position.z; // gl_FragCoord.z
    
    float dx = dfdx( linearDepth );
    float dy = dfdy( linearDepth );
    
    float moment1 = linearDepth;
    float moment2 = linearDepth * linearDepth + 0.25 * (dx * dx + dy * dy);

    return half4( moment1, moment2, 0.0, 0.0 );
}
