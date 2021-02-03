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
    matrix boneTransform = boneMatrices[ boneIndex.x ] * boneWeights.x + 
                           boneMatrices[ boneIndex.y ] * boneWeights.y +
                           boneMatrices[ boneIndex.z ] * boneWeights.z +
                           boneMatrices[ boneIndex.w ] * boneWeights.w;
    const float4 position2 = mul( boneTransform, float4( pos, 1.0f ) );
    
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
