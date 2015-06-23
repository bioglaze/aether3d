#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct
{
    matrix_float4x4 modelview_projection_matrix;
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
    float3 texCoords;
} ColorInOut;

constexpr sampler s(coord::normalized,
                    address::repeat,
                    filter::linear);

vertex ColorInOut skybox_vertex(device vertex_t* vertex_array [[ buffer(0) ]],
                                constant uniforms_t& uniforms [[ buffer(1) ]],
                                unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;
    
    float4 in_position = float4(float3(vertex_array[vid].position), 1.0);
    out.position = uniforms.modelview_projection_matrix * in_position;
    
    out.color = half4( vertex_array[vid].color );
    out.texCoords = float3(vertex_array[vid].position);
    return out;
}

fragment half4 skybox_fragment(ColorInOut in [[stage_in]],
                               texturecube<float, access::sample> texture [[texture(0)]])
{
    half4 sampledColor = half4(texture.sample(s, in.texCoords)) * in.color;
    return half4(sampledColor);
}
