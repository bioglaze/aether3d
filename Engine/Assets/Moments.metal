#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

#include "MetalCommon.h"

struct Vertex
{
    float3 position [[attribute(0)]];
    //int4 boneIndex [[attribute(5)]];
    //float4 boneWeights [[attribute(6)]];
};

struct ColorInOut
{
    float4 position [[position]];
};

vertex ColorInOut moments_vertex(Vertex vert [[stage_in]],
                               constant Uniforms& uniforms [[ buffer(5) ]])
{
    ColorInOut out;
    
    /*float4 in_position = float4( vert.position, 1.0 );
    
    float4 position2 = uniforms.boneMatrices[ vert.boneIndex.x ] * in_position * vert.boneWeights.x;
    position2 += uniforms.boneMatrices[ vert.boneIndex.y ] * in_position * vert.boneWeights.y;
    position2 += uniforms.boneMatrices[ vert.boneIndex.z ] * in_position * vert.boneWeights.z;
    position2 += uniforms.boneMatrices[ vert.boneIndex.w ] * in_position * vert.boneWeights.w;
    out.position = uniforms.localToClip * position2;*/

    float4 in_position = float4( vert.position, 1.0 );
    out.position = uniforms.localToClip * in_position;
    out.position.z = out.position.z * 0.5f + 0.5f; // -1..1 to 0..1 conversion
    return out;
}

fragment float4 moments_fragment( ColorInOut in [[stage_in]] )
{
    float linearDepth = in.position.z;

    float dx = dfdx( linearDepth );
    float dy = dfdy( linearDepth );
    
    float moment1 = linearDepth;
    // Compute second moment over the pixel extents.
    float moment2 = linearDepth * linearDepth + 0.25f * (dx * dx + dy * dy);

    return float4( moment1, moment2, 0.0, 0.0 );
}
