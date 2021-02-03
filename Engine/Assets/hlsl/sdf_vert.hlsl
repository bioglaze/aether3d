struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};
    
#include "ubo.h"
    
VSOutput main( float3 pos : POSITION, float2 uv : TEXCOORD, float4 color : COLOR )
{
    VSOutput vsOut;
    vsOut.pos = mul( localToClip, float4( pos, 1.0f ) );
    vsOut.uv = uv;
    vsOut.color = color;
    return vsOut;
}
