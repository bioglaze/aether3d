#include <metal_stdlib>
#include <simd/simd.h>

#define TILE_RES 16
#define LIGHT_INDEX_BUFFER_SENTINEL 0x7fffffff

using namespace metal;

struct StandardUniforms
{
    matrix_float4x4 _ModelViewProjectionMatrix;
    matrix_float4x4 _ShadowProjectionMatrix;
    float4 tintColor;
};

struct StandardColorInOut
{
    float4 position [[position]];
    float4 projCoord;
    float2 texCoords;
    half4  color;
};

struct StandardVertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float4 tangent [[attribute(3)]];
    float4 color [[attribute(4)]];
};

//layout (rgba32f, binding = 0) uniform imageBuffer pointLightBufferCenterAndRadius;
//layout (r32ui, binding = 2) uniform uimageBuffer perTileLightIndexBuffer;
//layout (rgba32f, binding = 3) uniform imageBuffer pointLightColors;

vertex StandardColorInOut standard_vertex(StandardVertex vert [[stage_in]],
                               constant StandardUniforms& uniforms [[ buffer(5) ]],
                               unsigned int vid [[ vertex_id ]])
{
    StandardColorInOut out;
    
    float4 in_position = float4( float3( vert.position ), 1.0 );
    out.position = uniforms._ModelViewProjectionMatrix * in_position;
    
    out.color = half4( vert.color );
    out.texCoords = float2( vert.texcoord );
    out.projCoord = uniforms._ShadowProjectionMatrix * in_position;
    return out;
}

fragment half4 standard_fragment( StandardColorInOut in [[stage_in]],
                               texture2d<float, access::sample> textureMap [[texture(0)]],
                               texture2d<float, access::sample> _ShadowMap [[texture(1)]],
                               sampler sampler0 [[sampler(0)]] )
{
    half4 sampledColor = half4( textureMap.sample( sampler0, in.texCoords ) );

    return sampledColor;
}
