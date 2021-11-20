#include "ubo.h"

float rand_1_05( float2 uv )
{
    float2 nois = (frac( sin( dot( uv, float2( 12.9898, 78.233) * 2.0 ) ) * 43758.5453 ) );
    return abs( nois.x + nois.y ) * 0.5;
}

[numthreads( 64, 1, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float2 uv = globalIdx.xy;// / float2( windowWidth, windowHeight );
    float ra = rand_1_05( uv );
    float x = 1;//rand_1_05( uv );
    //float4 position = float4( globalIdx.x * x * 8, sin( timeStamp * 10 ) * 16, 0, 1 );
    //float4 position = float4( globalIdx.x * x * 8 * sin( timeStamp * 10 ), 0, globalIdx.y * x * 8 * cos( timeStamp * 10 ), 1 );
    float4 position = float4(  x * 8 * sin( globalIdx.x * 10 + timeStamp ), globalIdx.x % 10,  x * 8 * cos( globalIdx.x * 10 + timeStamp ), 1 );
    particles[ globalIdx.x ].position = position;
    particles[ globalIdx.x ].color = float4( 1, 0, 0, 1 );
    particles[ globalIdx.x ].clipPosition = position;//mul( viewToClip, position );
}
