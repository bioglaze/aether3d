#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Variables in constant address space
//constant float3 light_position = float3(0.0, 1.0, -1.0);

typedef struct
{
    matrix_float4x4 _ProjectionModelMatrix;
} uniforms_t;

typedef struct
{
    packed_float3 position;
    packed_float2 texcoord;
    packed_float4 color;
} vertex_t;

typedef struct
{
    float4 position [[position]];
    half4  color;
    float2 texCoords;
} ColorInOut;

constexpr sampler s(coord::normalized,
                    address::repeat,
                    filter::linear);

vertex ColorInOut sprite_vertex(device vertex_t* vertex_array [[ buffer(0) ]],
                                  constant uniforms_t& uniforms [[ buffer(5) ]],
                                  unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;
    
    float4 in_position = float4(float3(vertex_array[vid].position), 1.0);
    out.position = uniforms._ProjectionModelMatrix * in_position;
    
    out.color = half4( vertex_array[vid].color );
    out.texCoords = vertex_array[vid].texcoord;
    return out;
}

fragment half4 sprite_fragment(ColorInOut in [[stage_in]],
                               texture2d<float, access::sample> texture [[texture(0)]])
{
    half4 sampledColor = half4(texture.sample(s, in.texCoords)) * in.color;
    return half4(sampledColor);
}
