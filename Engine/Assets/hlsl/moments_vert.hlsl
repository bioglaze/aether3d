struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

#include "ubo.h"

VSOutput main( float3 pos : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL )
{
    VSOutput vsOut;
    vsOut.uv = uv;
    vsOut.pos = mul( localToClip, float4( pos, 1.0f ) );
#if !VULKAN
    vsOut.pos.y = -vsOut.pos.y;
#endif
    
    if (lightType == 2) // Directional light.
    {
        vsOut.pos.z = vsOut.pos.z * 0.5f + 0.5f; // -1..1 to 0..1 conversion
    }

    return vsOut;
}
