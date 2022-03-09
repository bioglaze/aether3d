#include "ubo.h"

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float4 main( VSOutput vsOut ) : SV_Target
{
    float linearDepth = vsOut.pos.z;
    
    float dx = ddx( linearDepth );
    float dy = ddy( linearDepth );
    
    float moment1 = linearDepth;
    float moment2 = linearDepth * linearDepth + 0.25f * (dx * dx + dy * dy);
    
    float alphaTest = tex.Sample( sLinear, vsOut.uv ).r;

    return alphaTest > alphaThreshold ? float4( moment1, moment2, 0.0f, 1.0f ) : float4( 1, 1, 0, 0 );
}
