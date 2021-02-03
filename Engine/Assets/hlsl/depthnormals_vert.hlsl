struct VSOutput
{
    float4 pos : SV_Position;
    float3 mvPosition : POSITION;
    float3 normal : NORMAL;
};

#include "ubo.h"

VSOutput main( float3 pos : POSITION, float3 normal : NORMAL )
{
    VSOutput vsOut;
    vsOut.pos = mul( localToClip, float4( pos, 1.0 ) );
    vsOut.mvPosition = mul( localToView, float4( pos, 1.0 ) ).xyz;
    vsOut.normal = mul( localToView, float4( normal, 0.0 ) ).xyz;
    return vsOut;
}
