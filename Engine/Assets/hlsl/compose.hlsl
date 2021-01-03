#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

#include "ubo.h"

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float4 color = tex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) ) * specularTex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) );

    rwTexture[ globalIdx.xy ] = color;
}
