#include "ubo.h"

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 uv : TEXCOORD;
    float4 color : COLOR;
};

float4 main( VSOutput vsOut ) : SV_Target
{
    return texCube.Sample( sLinear, vsOut.uv );
}
