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
    float z;
};

vertex ColorInOut moments_vertex(Vertex vert [[stage_in]],
                               constant uniforms_t& uniforms [[ buffer(5) ]],
                               unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;
    
    float4 in_position = float4( float3( vert.position ), 1.0 );
    out.position = uniforms._ModelViewProjectionMatrix * in_position;
    out.z = out.position.z * 0.5 + 0.5; // OpenGL->D3D11 unit cube fixup.;
    return out;
}

fragment float4 moments_fragment( ColorInOut in [[stage_in]] )
{
    //float linearDepth = in.position.z;
    float linearDepth = in.z;
    
    float dx = dfdx( linearDepth );
    float dy = dfdy( linearDepth );
    
    float moment1 = linearDepth;
    // Compute second moment over the pixel extents.
    float moment2 = linearDepth * linearDepth + 0.25 * (dx * dx + dy * dy);

    return float4( moment1, moment2, 0.0, 0.0 );
}
