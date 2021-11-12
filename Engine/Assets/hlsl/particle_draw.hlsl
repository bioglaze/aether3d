#include "ubo.h"

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float4 color = rwTexture[ globalIdx.xy ];
    
    for (uint i = 0; i < particleCount; ++i)
    {
        float4 clipPos = mul( viewToClip, particles[ i ].position );    
#if !VULKAN
        clipPos.y = -clipPos.y;
#endif
        float3 ndc = clipPos.xyz / clipPos.w;
        float3 unscaledWindowCoords = 0.5f * ndc + float3( 0.5f, 0.5f, 0.5f );
        float3 windowCoords = float3( windowWidth * unscaledWindowCoords.x, windowHeight * unscaledWindowCoords.y, unscaledWindowCoords.z );

        float dist = distance( windowCoords.xy, globalIdx.xy );
        const float radius = 5;
        if (dist < radius)
        {
            color = particles[ i ].color;
        }
    }

    rwTexture[ globalIdx.xy ] = color; 
}
