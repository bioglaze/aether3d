#include "ubo.h"

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float4 color = rwTexture[ globalIdx.xy ];
    
    for (uint i = 0; i < particleCount; ++i)
    {
        float dist = distance( particles[ i ].clipPosition.xy, globalIdx.xy );
        const float radius = particles[ i ].clipPosition.z;
        
        if (dist < radius)
        {
            color = particles[ i ].color;
        }
    }

    rwTexture[ globalIdx.xy ] = color; 
}
