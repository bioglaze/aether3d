#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct
{
    matrix_float4x4 _ModelViewProjectionMatrix;
} uniforms_t;

typedef struct
{
    float4 position [[position]];
    half4  color;
    float3 texCoords;
} ColorInOut;

struct Vertex
{
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
    float4 color [[attribute(2)]];
};

constexpr sampler s(coord::normalized,
                    address::repeat,
                    filter::linear);

vertex ColorInOut skybox_vertex( Vertex vert [[stage_in]],
                                constant uniforms_t& uniforms [[ buffer(5) ]],
                                unsigned int vid [[ vertex_id ]])
{
    ColorInOut out;

    float4 in_position = float4( vert.position, 1.0 );
    out.position = uniforms._ModelViewProjectionMatrix * in_position;

    out.color = half4( vert.color );
    out.texCoords = vert.position;
    return out;
}

fragment half4 skybox_fragment(ColorInOut in [[stage_in]],
                               texturecube<float, access::sample> texture [[texture(0)]])
{
    half4 sampledColor = half4(texture.sample(s, in.texCoords)) * in.color;
    return half4(sampledColor);
}
