#if !VULKAN
#define layout(a,b)  
#endif

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

#include "ubo.h"

VSOutput main( uint vertexId : SV_VertexID )
{
    VSOutput vsOut;
    vsOut.uv = float2( (vertexId << 1) & 2, vertexId & 2 );
    vsOut.pos = float4( vsOut.uv * 2.0f + -1.0f, 0.0f, 1.0f );
    
    vsOut.uv.y = 1.0f - vsOut.uv.y;

    vsOut.color = float4( 1.0f, 1.0f, 1.0f, 1.0f );
    return vsOut;
}
