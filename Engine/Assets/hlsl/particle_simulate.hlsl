#include "ubo.h"

float rand_1_05( float2 uv )
{
    float2 nois = (frac( sin( dot( uv, float2(12.9898, 78.233) * 2.0 ) ) * 43758.5453 ));
    return abs( nois.x + nois.y ) * 0.5;
}

bool3 greaterThan( float3 a, float3 b )
{
    return bool3( a.x > b.x, a.y > b.y, a.z > b.z );
}

[numthreads( 64, 1, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    const uint i = globalIdx.x;
    float2 uv = globalIdx.xy;
    float ra = rand_1_05( uv );
    float x = 1;
    float4 position = float4(x * 8 * sin( globalIdx.x * 20 + timeStamp ), globalIdx.x % 20, x * 8 * cos( globalIdx.x * 20 + timeStamp ), 1 );

    particles[ i ].positionAndSize = position;
    particles[ i ].positionAndSize.w = 5;
    particles[ i ].color = particleColor;

    float4 clipPos = mul( viewToClip, position );

#if !VULKAN
    clipPos.y = -clipPos.y;
#endif
    if (any( greaterThan( abs( clipPos.xyz ), float3( abs( clipPos.www ) ) ) ))
    {
        particles[ i ].clipPosition = float4( 0, 0, 0, 666 );
        return;
    }
    
    float3 ndc = clipPos.xyz / clipPos.w;
    float3 unscaledWindowCoords = 0.5f * ndc + float3(0.5f, 0.5f, 0.5f);
    float3 windowCoords = float3(windowWidth * unscaledWindowCoords.x, windowHeight * unscaledWindowCoords.y, unscaledWindowCoords.z);

    particles[ i ].clipPosition = float4( windowCoords.x, windowCoords.y, clipPos.z, 1 );
}
