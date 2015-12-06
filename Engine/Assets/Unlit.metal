#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

float linstep( float low, float high, float v );
float VSM( texture2d<float, access::sample> shadowMap, float4 projCoord, float depth );

typedef struct
{
    matrix_float4x4 _ModelViewProjectionMatrix;
    float4 tintColor;
    matrix_float4x4 _ShadowProjectionMatrix;
} uniforms_t;

typedef struct
{
    packed_float3 position;
    packed_float2 texcoord;
    packed_float3 normal;
    packed_float4 tangent;
    packed_float4 color;
} vertex_t;

/*typedef struct
{
    packed_float3 position;
    packed_float2 texcoord;
    packed_float4 color;
} vertex_t;*/

typedef struct
{
    float4 position [[position]];
    half4  color;
    float2 texCoords;
    float4 tintColor;
} ColorInOut;

constexpr sampler s(coord::normalized,
                    address::repeat,
                    filter::linear);

float linstep( float low, float high, float v )
{
    return clamp( (v - low) / (high - low), 0.0, 1.0 );
}

float VSM( texture2d<float, access::sample> shadowMap, float4 projCoord, float depth )
{
    float2 moments = shadowMap.sample( s, projCoord.xy / projCoord.w ).rg;
    
    float variance = max( moments.y - moments.x * moments.x, -0.001 );
    
    float delta = depth - moments.x;
    float p = smoothstep( depth - 0.02, depth, moments.x );
    float pMax = linstep( 0.2, 1.0, variance / (variance + delta * delta) );
    
    return clamp( max( p, pMax ), 0.0, 1.0 );
}

vertex ColorInOut unlit_vertex(device vertex_t* vertex_array [[ buffer(0) ]],
                                constant uniforms_t& uniforms [[ buffer(1) ]],
                                unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;
    
    float4 in_position = float4( float3( vertex_array[ vid ].position ), 1.0 );
    out.position = uniforms._ModelViewProjectionMatrix * in_position;
    
    out.color = half4( vertex_array[vid].color );
    out.texCoords = float2( vertex_array[ vid ].texcoord );
    out.tintColor = uniforms.tintColor;
    return out;
}

fragment half4 unlit_fragment( ColorInOut in [[stage_in]],
                               texture2d<float, access::sample> textureMap [[texture(0)]],
                               texture2d<float, access::sample> shadowMap [[texture(1)]] )
{
    half4 sampledColor = half4( textureMap.sample( s, in.texCoords ) ) * half4( in.tintColor );

    //float depth = 1.0;//vProjCoord.z / vProjCoord.w;
    //float4 shadow = VSM( shadowMap, float4( 0.0, 0.0, 0.0, 0.0 ), depth );

    return half4( sampledColor );// * half4( shadow );
}
