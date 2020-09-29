#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

struct VSOutput
{
    float4 pos : SV_Position;
    float3 mvPosition : POSITION;
    float3 normal : NORMAL;
};

#include "ubo.h"

VSOutput main( float3 pos : POSITION, float2 uv : TEXCOORD, float3 normal : NORMAL, float4 tangent : TANGENT, float4 color : COLOR, float4 boneWeights : WEIGHTS, uint4 boneIndex : BONES )
{
    VSOutput vsOut;
    float4 position2 = mul( boneMatrices[ boneIndex.x ], float4( pos, 1.0f ) ) * boneWeights.x;
    position2 += mul( boneMatrices[ boneIndex.y ], float4( pos, 1.0f ) ) * boneWeights.y;
    position2 += mul( boneMatrices[ boneIndex.z ], float4( pos, 1.0f ) ) * boneWeights.z;
    position2 += mul( boneMatrices[ boneIndex.w ], float4( pos, 1.0f ) ) * boneWeights.w;

    vsOut.pos = mul( localToClip, position2 );
    vsOut.mvPosition = mul( localToView, float4( pos, 1.0 ) ).xyz;
    vsOut.normal = mul( localToView, float4( normal, 0.0 ) ).xyz;
    return vsOut;
}
