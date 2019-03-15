#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    matrix_float4x4 localToClip;
};

struct ColorInOut
{
    float4 position [[position]];
    //half4  color;
    float3 texCoords;
};

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
};

constexpr sampler samp( coord::normalized, address::repeat, filter::linear );

vertex ColorInOut skybox_vertex( Vertex vert [[stage_in]],
                                constant Uniforms& uniforms [[ buffer(5) ]])
{
    ColorInOut out;

    float4 in_position = float4( vert.position, 1.0 );
    out.position = uniforms.localToClip * in_position;

    out.texCoords = vert.position;
    return out;
}

fragment half4 skybox_fragment( ColorInOut in [[stage_in]],
                                texturecube<float, access::sample> texture [[texture(0)]] )
{
    half4 sampledColor = half4( texture.sample( samp, in.texCoords ) );
    return half4(sampledColor);
}
