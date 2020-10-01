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
    matrix boneTransform = boneMatrices[ boneIndex.x ] * boneWeights.x + 
                           boneMatrices[ boneIndex.y ] * boneWeights.y +
                           boneMatrices[ boneIndex.z ] * boneWeights.z +
                           boneMatrices[ boneIndex.w ] * boneWeights.w;
    const float4 position2 = mul( boneTransform, float4( pos, 1.0f ) );
    const float4 normal2 = mul( boneTransform, float4( normal, 0.0f ) );

    vsOut.pos = mul( localToClip, position2 );
    vsOut.mvPosition = mul( localToView, position2 ).xyz;
    vsOut.normal = mul( localToView, normal2 ).xyz;
    return vsOut;
}
