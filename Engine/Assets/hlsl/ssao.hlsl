#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

#include "ubo.h"

float ssao( int sampleCount, uint2 globalIdx )
{
    float4 depthNormals = normalTex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) );
    
    if (depthNormals.x >= 1)
    {
        return 1;
    }

    float4 projWorldPos = clipToView * float4( globalIdx.x, globalIdx.y, depthNormals.x * 2 - 1, 1 );
    float3 worldPos = projWorldPos.xyz / projWorldPos.w;

    int num = sampleCount;
    
    for (int i = 0; i < sampleCount; ++i )
    {
        float3 p = worldPos + kernelOffsets[ i ] * 10;
        float4 proj = viewToClip * float4( p, 1 );
        proj.xy /= proj.w;
        proj.z = (proj.z - 0.005f) / proj.w;
        proj.xyz = proj.xyz * 0.5f + 0.5f;
        float depth = normalTex.Load( uint3( proj.xy * float2( 1920, 1080 ), 0 ) ).x;

        if (depth < proj.z)
        {
            --num;
        }
    }

    float ao = float( num ) / float( sampleCount );
    return ao;
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float4 color = tex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) ) * ssao( 64, globalIdx.xy );

    rwTexture[ globalIdx.xy ] = color;
}
