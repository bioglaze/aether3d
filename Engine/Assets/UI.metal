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
    float2 texCoords;
    float4 color;
};

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float4 color [[attribute(2)]];
};

vertex ColorInOut ui_vertex(Vertex vert [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(5) ]])
{
    ColorInOut out;

    out.position = uniforms.localToClip * float4( vert.position, 1.0 );
    out.color = vert.color;
    out.texCoords = vert.texcoord;
    return out;
}

fragment float4 ui_fragment( ColorInOut in [[stage_in]],
                               texture2d<float, access::sample> textureMap [[texture(0)]],
                               sampler sampler0 [[sampler(0)]] )
{
    return textureMap.sample( sampler0, in.texCoords ) * in.color;
}
