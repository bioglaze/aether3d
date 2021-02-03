struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float4 projCoord : TANGENT;
};
    
#include "ubo.h"

VSOutput main( float3 pos : POSITION, float2 uv : TEXCOORD, float4 color : COLOR )
{
    VSOutput vsOut;
    vsOut.pos = mul( localToClip, float4( pos, 1.0 ) );
    
    if (isVR == 1)
    {
        vsOut.pos.y = -vsOut.pos.y;
    }

    vsOut.uv = uv;
    vsOut.color = color;
    vsOut.projCoord = mul( localToShadowClip, float4( pos, 1.0 ) );
    return vsOut;
}
