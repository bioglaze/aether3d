#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

#include "ubo.h"

layout( set = 0, binding = 1 ) Texture2D colorTex : register(t0);
layout( set = 0, binding = 2 ) Buffer<float4> unused : register(t0);
layout( set = 0, binding = 11 ) RWTexture2D<float4> downsampledBrightTexture : register(u0);

groupshared uint helper[ 128 ];

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    const float4 color = colorTex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) );
    const float luminance = dot( color.rgb, float3( 0.2126f, 0.7152f, 0.0722f ) );
    const float luminanceThreshold = 0.8f;
    const float4 finalColor = luminance > luminanceThreshold ? color : float4( 0, 0, 0, 0 );

    downsampledBrightTexture[ globalIdx.xy ] = finalColor;
}
