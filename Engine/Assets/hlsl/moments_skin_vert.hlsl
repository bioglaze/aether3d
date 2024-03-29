struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

#include "ubo.h"

VSOutput main( float3 pos : POSITION, float2 uv : TEXCOORD, float3 nor : NORMAL, float4 tangent : TANGENT, float4 color : COLOR, float4 boneWeights : WEIGHTS, uint4 boneIndex : BONES )
{
    VSOutput vsOut;
    float4 position2 = mul( boneMatrices[ boneIndex.x ], float4( pos, 1.0f ) ) * boneWeights.x;
    position2 += mul( boneMatrices[ boneIndex.y ], float4( pos, 1.0f ) ) * boneWeights.y;
    position2 += mul( boneMatrices[ boneIndex.z ], float4( pos, 1.0f ) ) * boneWeights.z;
    position2 += mul( boneMatrices[ boneIndex.w ], float4( pos, 1.0f ) ) * boneWeights.w;
    vsOut.pos = mul( localToClip, position2 );
    vsOut.uv = uv;

#if !VULKAN
    vsOut.pos.y = -vsOut.pos.y;
#endif
    
    if (lightType == 2)
    {
        // Disabled to get animatedGO's shadow working on Vulkan on Linux
        //        vsOut.pos.z = vsOut.pos.z * 0.5f + 0.5f; // -1..1 to 0..1 conversion
    }

    return vsOut;
}
