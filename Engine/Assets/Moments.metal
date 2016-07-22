#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct uniforms_t
{
    matrix_float4x4 _ModelViewProjectionMatrix;
};

struct Vertex
{
    float3 position [[attribute(0)]];
};

struct ColorInOut
{
    float4 position [[position]];
};

vertex ColorInOut moments_vertex(Vertex vert [[stage_in]],
                               constant uniforms_t& uniforms [[ buffer(5) ]],
                               unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;
    
    float4 in_position = float4( float3( vert.position ), 1.0 );
    out.position = uniforms._ModelViewProjectionMatrix * in_position;
    
    return out;
}

fragment half4 moments_fragment( ColorInOut in [[stage_in]] )
{
    float linearDepth = in.position.z;
    
    float dx = dfdx( linearDepth );
    float dy = dfdy( linearDepth );
    
    float moment1 = linearDepth;
    float moment2 = linearDepth * linearDepth + 0.25 * (dx * dx + dy * dy);

    return half4( moment1, moment2, 0.0, 0.0 );
}
