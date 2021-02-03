#include "ubo.h"

groupshared uint helper[ 128 ];

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    const float4 color = tex.Load( uint3( globalIdx.x * 2, globalIdx.y * 2, 0 ) );
    const float luminance = dot( color.rgb, float3( 0.2126f, 0.7152f, 0.0722f ) );
    const float luminanceThreshold = bloomParams.x;
    const float4 finalColor = luminance > luminanceThreshold ? color : float4( 0, 0, 0, 0 );

    rwTexture[ globalIdx.xy ] = finalColor;
}
