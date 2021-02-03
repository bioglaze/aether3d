struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 uv : TEXCOORD;
    float4 color : COLOR;
};

#include "ubo.h"

VSOutput main( float3 pos : POSITION, float2 uv : TEXCOORD, float4 color : COLOR )
{
    VSOutput vsOut;
    vsOut.pos = mul( localToClip, float4( pos, 1.0 ) );
    vsOut.uv = pos;
    vsOut.color = color;
    return vsOut;
}
