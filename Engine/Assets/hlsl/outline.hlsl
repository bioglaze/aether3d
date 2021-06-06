#include "ubo.h"

groupshared uint helper[ 128 ];

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    // Algorithm source: https://ourmachinery.com/post/borderland-part-3-selection-highlighting/
    float4 id0 = tex.GatherRed( sLinear, globalIdx.xy / float2( windowWidth, windowHeight ), int2( -2, -2 ) );
    float4 id1 = tex.GatherRed( sLinear, globalIdx.xy / float2( windowWidth, windowHeight ), int2( 0, -2 ) );
    float4 id2 = tex.GatherRed( sLinear, globalIdx.xy / float2( windowWidth, windowHeight ), int2( -2, 0 ) );
    float4 id3 = tex.GatherRed( sLinear, globalIdx.xy / float2( windowWidth, windowHeight ), int2( 0, 0 ) );

    id2.xw = id1.xy;
    float idCenter = id3.w;
    id3.w = id0.y;

    static const float avg_scalar = 1.f / 8.f;
    float a = dot( float4( id2 != idCenter ), avg_scalar );
    a += dot( float4( id3 != idCenter ), avg_scalar );

    rwTexture[ globalIdx.xy ] = a;
}
