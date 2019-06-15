#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    float4 projCoord : TANGENT;
};
    
#include "ubo.h"

VSOutput main( float3 pos : POSITION, float2 uv : TEXCOORD, float3 nor : NORMAL, float4 tangent : TANGENT, float4 color : COLOR, float4 boneWeights : WEIGHTS, uint4 boneIndex : BONES )
{
    float4 position2 = mul( boneMatrices[ boneIndex.x ], float4( pos, 1.0 ) ) * boneWeights.x;
    position2 += mul( boneMatrices[ boneIndex.y ], float4( pos, 1.0 ) ) * boneWeights.y;
    position2 += mul( boneMatrices[ boneIndex.z ], float4( pos, 1.0 ) ) * boneWeights.z;
    position2 += mul( boneMatrices[ boneIndex.w ], float4( pos, 1.0 ) ) * boneWeights.w;

    VSOutput vsOut;
    vsOut.pos = mul( localToClip, position2 );

    if (isVR == 1)
    {
        vsOut.pos.y = -vsOut.pos.y;
    }

    vsOut.uv = uv;
    vsOut.color = color;
    vsOut.projCoord = mul( localToShadowClip, position2 );
    return vsOut;
}
