#include "ubo.h"

groupshared uint helper[ 128 ];

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float weights[ 5 ] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
    float4 accumColor = tex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) ) * weights[ 0 ];

    for (int x = 1; x < 5; ++x)
    {
        accumColor += tex.Load( uint3(globalIdx.x + x * tilesXY.z, globalIdx.y + x * tilesXY.w, 0) ) * weights[ x ];
        accumColor += tex.Load( uint3( globalIdx.x - x * tilesXY.z, globalIdx.y - x * tilesXY.w, 0 ) ) * weights[ x ];
    }

    rwTexture[ globalIdx.xy ] = accumColor;
}
