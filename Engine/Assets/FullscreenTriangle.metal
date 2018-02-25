#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct ColorInOut
{
    float4 position [[position]];
    float2 texCoords;
};

vertex ColorInOut fullscreen_triangle_vertex( uint vid [[ vertex_id ]] )
{
    ColorInOut out;

    out.texCoords = float2( (vid << 1) & 2, vid & 2 );
    out.position = float4( out.texCoords * 2.0f + -1.0f, 0.0f, 1.0f );
    return out;
}

fragment float4 fullscreen_triangle_fragment( ColorInOut in [[stage_in]],
                               texture2d<float, access::sample> textureMap [[texture(0)]],
                               sampler sampler0 [[sampler(0)]] )
{
    return textureMap.sample( sampler0, float2( in.texCoords.x, -in.texCoords.y ) );
}
