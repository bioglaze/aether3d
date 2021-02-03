struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

#include "ubo.h"

float4 main( VSOutput vsOut ) : SV_Target
{
    return tex.Sample( sLinear, vsOut.uv ) * vsOut.color;
}
