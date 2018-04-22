#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct Uniforms
{
    matrix_float4x4 localToClip;
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

vertex ColorInOut sdf_vertex( Vertex vert [[stage_in]],
                              constant Uniforms& uniforms [[ buffer(5) ]])
{
    ColorInOut out;
    
    float4 in_position = float4( vert.position, 1.0 );
    out.position = uniforms.localToClip * in_position;
    
    out.color = half4( vert.color );
    out.texCoords = vert.texcoord;
    return out;
}

fragment half4 sdf_fragment( ColorInOut in [[stage_in]],
                             texture2d<float, access::sample> texture [[texture(0)]] )
{
    const float edgeDistance = 0.5;
    float distance = half4(texture.sample(s, in.texCoords)).r;
    float edgeWidth = 0.7f * length( float2( dfdx( distance ), dfdy( distance ) ) );
    float opacity = smoothstep( edgeDistance - edgeWidth, edgeDistance + edgeWidth, distance );

    return half4( in.color.rgb, opacity );
}
