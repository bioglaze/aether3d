#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#include "MetalCommon.h"

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
                               texturecube<float, access::sample> _ShadowMapCube [[texture(2)]],
                               constant Uniforms& uniforms [[ buffer(5) ]],
                               sampler sampler0 [[sampler(0)]] )
{
    float4 sampledColor = textureMap.sample( sampler0, in.texCoords ) * in.tintColor;

    float depth = in.projCoord.z / in.projCoord.w;

    if (uniforms.lightType == 2)
    {
        depth = depth * 0.5f + 0.5f;
    }
    
    float shadow = uniforms.lightType == 0 ? 1.0f : max( 0.2f, VSM( _ShadowMap, in.projCoord, depth, uniforms.lightType ) );
    
    return sampledColor * float4( shadow, shadow, shadow, 1 );
}
