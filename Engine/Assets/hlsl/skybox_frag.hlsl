struct VSOutput
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 uv : TEXCOORD;
};

#include "ubo.h"

float4 main( VSOutput vsOut ) : SV_Target
{
    return texCube.Sample( sLinear, vsOut.uv );
}
