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

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float4 color [[attribute(2)]];
    
    int4 boneIndex [[attribute(5)]];
    float4 boneWeights [[attribute(6)]];
};

vertex ColorInOut unlit_skin_vertex(Vertex vert [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(5) ]])
{
    ColorInOut out;

    float4 in_position = float4( vert.position, 1.0 );
    
    float4 position2 = uniforms.boneMatrices[ vert.boneIndex.x ] * in_position * vert.boneWeights.x;
    position2 += uniforms.boneMatrices[ vert.boneIndex.y ] * in_position * vert.boneWeights.y;
    position2 += uniforms.boneMatrices[ vert.boneIndex.z ] * in_position * vert.boneWeights.z;
    position2 += uniforms.boneMatrices[ vert.boneIndex.w ] * in_position * vert.boneWeights.w;
    out.position = uniforms.localToClip * position2;

    out.color = half4( vert.color );
    out.texCoords = vert.texcoord;
    out.tintColor = float4( 1, 1, 1, 1 );
    out.projCoord = uniforms.localToShadowClip * in_position;
    return out;
}

fragment float4 unlit_skin_fragment( ColorInOut in [[stage_in]],
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
