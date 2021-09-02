#include "ubo.h"

[numthreads( 64, 1, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float4 position = float4(5, 0, 0, 1);
    particles[ globalIdx.x ].position = position;
    particles[ globalIdx.x ].color = float4( 1, 0, 0, 1 );
    particles[ globalIdx.x ].clipPosition = mul( viewToClip, position );
}
