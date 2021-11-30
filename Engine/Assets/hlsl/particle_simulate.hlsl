#include "ubo.h"

float rand_1_05( float2 uv )
{
    float2 nois = (frac( sin( dot( uv, float2(12.9898, 78.233) * 2.0 ) ) * 43758.5453 ));
    return abs( nois.x + nois.y ) * 0.5;
}

[numthreads( 64, 1, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    const uint i = globalIdx.x;
    float2 uv = globalIdx.xy;
    float ra = rand_1_05( uv );
    float x = 1;
    float4 position = float4(x * 8 * sin( globalIdx.x * 20 + timeStamp ), globalIdx.x % 20, x * 8 * cos( globalIdx.x * 20 + timeStamp ), 1);
    particles[ i ].position = position;
    particles[ i ].color = float4(1, 1, 1, 1);

    float4 clipPos = mul( viewToClip, position );

#if !VULKAN
    clipPos.y = -clipPos.y;
#endif
    float3 ndc = clipPos.xyz / clipPos.w;
    float3 unscaledWindowCoords = 0.5f * ndc + float3(0.5f, 0.5f, 0.5f);
    float3 windowCoords = float3(windowWidth * unscaledWindowCoords.x, windowHeight * unscaledWindowCoords.y, unscaledWindowCoords.z);

    particles[ i ].clipPosition = float4(windowCoords.x, windowCoords.y, 5, 1);
}
