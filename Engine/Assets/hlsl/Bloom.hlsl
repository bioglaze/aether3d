#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

#include "ubo.h"

layout( set = 0, binding = 1 ) Texture2D colorTex : register(t0);
layout( set = 0, binding = 2 ) RWTexture2D<float4> bloomTex : register(u0);

groupshared uint ldsLightIdx[ 128 ];

float luminance( float3 color )
{
    return 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float threshold = 0.8f;
    if (luminance( colorTex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) ).rgb ) > threshold)
    {
        bloomTex[ globalIdx.xy ] = float4(0, 1, 0, 1);
    }

    bloomTex[ globalIdx.xy ] = colorTex.Load( uint3(globalIdx.x, globalIdx.y, 0) );
}
