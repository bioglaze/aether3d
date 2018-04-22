#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#include "MetalCommon.h"

float linstep( float low, float high, float v );
float VSM( texture2d<float, access::sample> shadowMap, float4 projCoord, float depth );

struct ColorInOut
{
    float4 position [[position]];
    float4 tintColor;
    float4 projCoord;
    float2 texCoords;
    half4  color;
};

constexpr sampler shadowSampler(coord::normalized,
                                address::clamp_to_zero,
                                filter::linear);

float linstep( float low, float high, float v )
{
    return clamp( (v - low) / (high - low), 0.0f, 1.0f );
}

float VSM( texture2d<float, access::sample> shadowMap, float4 projCoord, float depth )
{
    float2 uv = (projCoord.xy / projCoord.w) * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;
    
    float2 moments = shadowMap.sample( shadowSampler, uv ).rg;
    
    float variance = max( moments.y - moments.x * moments.x, -0.001 );

    float delta = depth - moments.x;
    float p = smoothstep( depth - 0.02f, depth, moments.x );
    float pMax = linstep( 0.2f, 1.0f, variance / (variance + delta * delta) );
    
    return clamp( max( p, pMax ), 0.0f, 1.0f );
}

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float4 color [[attribute(2)]];
};

vertex ColorInOut unlit_vertex(Vertex vert [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(5) ]])
{
    ColorInOut out;

    float4 in_position = float4( vert.position, 1.0 );
    
    out.position = uniforms.localToClip * in_position;
    out.color = half4( vert.color );
    out.texCoords = vert.texcoord * uniforms.tex0scaleOffset.xy + uniforms.tex0scaleOffset.wz;
    out.tintColor = float4( 1, 1, 1, 1 );
    out.projCoord = uniforms.localToShadowClip * in_position;
    return out;
}

fragment float4 unlit_fragment( ColorInOut in [[stage_in]],
                               texture2d<float, access::sample> textureMap [[texture(0)]],
                               texture2d<float, access::sample> _ShadowMap [[texture(1)]],
                               sampler sampler0 [[sampler(0)]] )
{
    float4 sampledColor = textureMap.sample( sampler0, in.texCoords ) * in.tintColor;

    float depth = in.projCoord.z / in.projCoord.w;
    depth = depth * 0.5f + 0.5f;
    float shadow = max( 0.2f, VSM( _ShadowMap, in.projCoord, depth ) );
    
    return sampledColor * float4( shadow, shadow, shadow, 1 );
}
