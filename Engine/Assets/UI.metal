#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

float linstep( float low, float high, float v );
float VSM( texture2d<float, access::sample> shadowMap, float4 projCoord, float depth );

struct Uniforms
{
    matrix_float4x4 projection;
};

struct ColorInOut
{
    float4 position [[position]];
    float4 tintColor;
    float4 projCoord;
    float2 texCoords;
    half4  color;
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

    float4 in_position = float4( vert.position, 1.0 );
    out.position = uniforms.projection * in_position;
    
    out.color = half4( vert.color );
    out.texCoords = vert.texcoord;
    return out;
}

fragment float4 ui_fragment( ColorInOut in [[stage_in]],
                               texture2d<float, access::sample> textureMap [[texture(0)]],
                               texture2d<float, access::sample> _ShadowMap [[texture(1)]],
                               sampler sampler0 [[sampler(0)]] )
{
    float4 sampledColor = textureMap.sample( sampler0, in.texCoords ) * in.tintColor;
    return sampledColor;
}
