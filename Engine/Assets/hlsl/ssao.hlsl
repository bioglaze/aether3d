#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

#include "ubo.h"

float ssao( int sampleCount, uint2 globalIdx )
{
    float acc = 0;

    float4 depthNormals = normalTex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) );
    
    if (deptNormals.x >= 1)
    {
        return 1;
    }

    float4 projWorldPos = clipToView * float4( globalIdx.x, globalIdx,y, depthNormals.x * 2 - 1, 1 );
    float3 worldPos = projWorldPos.xyz / projWorldPos.w;

    int num = sampleCount;
    
    for (int i = 0; i < sampleCount; ++i )
    {
        vec3 p = worldPos + kernelOffsets[ i ] * 10;
        // float4( kernelOffsets[ i ], 0 )
        acc += depthNormals.x;
    }

    float ao = float( num ) / float( sampleCount );
    return acc;
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float4 color = tex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) ) * ssao( 64, globalIdx.xy );

    rwTexture[ globalIdx.xy ] = color;
}
