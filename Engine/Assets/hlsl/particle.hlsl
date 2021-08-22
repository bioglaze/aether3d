#include "ubo.h"

[numthreads( 64, 1, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    particles[ globalIdx.x ].position = float4( 5, 0, 0, 1 );
}
