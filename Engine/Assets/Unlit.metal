#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

float linstep( float low, float high, float v );
float VSM( texture2d<float, access::sample> shadowMap, float4 projCoord, float depth );

struct uniforms_t
{
    matrix_float4x4 _ModelViewProjectionMatrix;
    matrix_float4x4 _ShadowProjectionMatrix;
    float4 tint;
};

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
    return clamp( (v - low) / (high - low), 0.0, 1.0 );
}

float VSM( texture2d<float, access::sample> shadowMap, float4 projCoord, float depth )
{
    float2 moments = shadowMap.sample( shadowSampler, projCoord.xy / projCoord.w ).rg;

    float variance = max( moments.y - moments.x * moments.x, -0.001 );

    float delta = depth - moments.x;
    float p = smoothstep( depth - 0.02, depth, moments.x );
    float pMax = linstep( 0.2, 1.0, variance / (variance + delta * delta) );
    
    //return clamp( max( p, pMax ), 0.0, 1.0 );
    return 1;//(depth <= moments.x) ? 0.2 : 1.0;
}

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float4 color [[attribute(2)]];
};

vertex ColorInOut unlit_vertex(Vertex vert [[stage_in]],
                               constant uniforms_t& uniforms [[ buffer(5) ]])
{
    ColorInOut out;
    
    float4 in_position = float4( vert.position, 1.0 );
    out.position = uniforms._ModelViewProjectionMatrix * in_position;
    
    out.color = half4( vert.color );
    out.texCoords = float2( vert.texcoord );
    out.tintColor = uniforms.tint;
    out.projCoord = uniforms._ShadowProjectionMatrix * in_position;
    return out;
}

fragment float4 unlit_fragment( ColorInOut in [[stage_in]],
                               texture2d<float, access::sample> textureMap [[texture(0)]],
                               texture2d<float, access::sample> _ShadowMap [[texture(1)]],
                               sampler sampler0 [[sampler(0)]] )
{
    float4 sampledColor = textureMap.sample( sampler0, in.texCoords ) * in.tintColor;

    float depth = in.projCoord.z / in.projCoord.w;
    float shadow = max( 0.2, VSM( _ShadowMap, in.projCoord, depth ) );
    
    return sampledColor * float4( shadow, shadow, shadow, 1 );
}
