#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

#include "ubo.h"

layout( set = 0, binding = 1 ) Texture2D inputTexture : register(t0);
layout( set = 0, binding = 2 ) Buffer<float4> unused : register(t0);
layout( set = 0, binding = 11 ) RWTexture2D<float4> blurTex : register(u0);

groupshared uint helper[ 128 ];

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float weights[ 5 ] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
    float4 accumColor = inputTexture.Load( uint3( globalIdx.x, globalIdx.y, 0 ) ) * weights[ 0 ];

    for (int x = 1; x < 5; ++x)
    {
        accumColor += inputTexture.Load( uint3(globalIdx.x + x * tilesXY.z, globalIdx.y + x * tilesXY.w, 0) ) * weights[ x ];
        accumColor += inputTexture.Load( uint3( globalIdx.x - x * tilesXY.z, globalIdx.y - x * tilesXY.w, 0 ) ) * weights[ x ];
    }

    blurTex[ globalIdx.xy ] = accumColor;
}
