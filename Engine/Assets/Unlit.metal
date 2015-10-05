#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct
{
    matrix_float4x4 _ModelViewProjectionMatrix;
    float4 tintColor;
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

vertex ColorInOut unlit_vertex(device vertex_t* vertex_array [[ buffer(0) ]],
                                constant uniforms_t& uniforms [[ buffer(1) ]],
                                unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;
    
    float4 in_position = float4(float3(vertex_array[vid].position), 1.0);
    out.position = uniforms._ModelViewProjectionMatrix * in_position;
    
    out.color = half4( vertex_array[vid].color );
    out.texCoords = float2(vertex_array[vid].texcoord);
    out.tintColor = uniforms.tintColor;
    return out;
}

fragment half4 unlit_fragment(ColorInOut in [[stage_in]],
                               texture2d<float, access::sample> textureMap [[texture(0)]])
{
    half4 sampledColor = half4(textureMap.sample(s, in.texCoords)) * half4( in.tintColor );
    return half4(sampledColor);
}
