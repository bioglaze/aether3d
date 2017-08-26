#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    matrix_float4x4 _LocalToClip;
};

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float4 color [[attribute(2)]];
};

struct ColorInOut
{
    float4 position [[position]];
    half4  color;
    float2 texCoords;
};

constexpr sampler s(coord::normalized,
                    address::repeat,
                    filter::linear);

vertex ColorInOut sprite_vertex(Vertex vert [[stage_in]],
                                constant Uniforms& uniforms [[ buffer(5) ]])
{
    ColorInOut out;
    
    float4 in_position = float4( float3( vert.position ), 1.0 );
    out.position = uniforms._LocalToClip * in_position;
    
    out.color = half4( vert.color );
    out.texCoords = vert.texcoord;
    return out;
}

fragment half4 sprite_fragment(ColorInOut in [[stage_in]],
                               texture2d<float, access::sample> texture [[texture(0)]])
{
    half4 sampledColor = half4( texture.sample( s, in.texCoords ) ) * in.color;
    return half4( sampledColor );
}
