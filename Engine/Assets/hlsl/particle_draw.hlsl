#include "ubo.h"

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float4 color = rwTexture[ globalIdx.xy ];
    
    for (int i = 0; i < particleCount; ++i)
    {
        if (particles[ i ].clipPosition.x == globalIdx.x && particles[ i ].clipPosition.y == globalIdx.y)
        {
            color += particles[ i ].color;
        }
    }

    rwTexture[ globalIdx.xy ] = color; 
}
