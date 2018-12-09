#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

#include "ubo.h"

layout( set = 0, binding = 1 ) Texture2D inputTexture : register(t0);
layout( set = 0, binding = 2 ) Buffer<float4> unused : register(t0);
layout( set = 0, binding = 11 ) RWTexture2D<float4> blurTex : register(u11);

groupshared uint helper[ 128 ];

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    const float weights[ 9 ] = { 0.000229f, 0.005977f, 0.060598f,
    0.241732f, 0.382928f, 0.241732f,
    0.060598f, 0.005977f, 0.000229f };

    float4 accumColor = float4( 0, 0, 0, 0 );

    for (int x = 0; x < 9; ++x)
    {
        const float4 color = inputTexture.Load( uint3( globalIdx.x + x * tilesXY.z, globalIdx.y + x * tilesXY.w, 0 ) ) * weights[ x ];
        accumColor += color;
    }

    blurTex[ globalIdx.xy ] = accumColor;
}
