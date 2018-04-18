#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

struct VSOutput
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 uv : TEXCOORD;
};

#include "ubo.h"

VSOutput main( float3 pos : POSITION, float2 uv : TEXCOORD, float4 color : COLOR )
{
    VSOutput vsOut;
    vsOut.pos = mul( localToClip, float4( pos, 1.0f ) );
    vsOut.color = color;
    vsOut.uv = pos.xyz;
    return vsOut;
}
