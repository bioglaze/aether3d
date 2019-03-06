#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

struct VSOutput
{
    float4 pos : SV_Position;
};

#include "ubo.h"

VSOutput main( float3 pos : POSITION, float3 normal : NORMAL, float4 boneWeights : WEIGHTS, uint4 boneIndex : BONES )
{
    VSOutput vsOut;
    float4 position2 = mul( boneMatrices[ boneIndex.x ], float4(pos, 1.0) ) * boneWeights.x;
    position2 += mul( boneMatrices[ boneIndex.y ], float4(pos, 1.0) ) * boneWeights.y;
    position2 += mul( boneMatrices[ boneIndex.z ], float4(pos, 1.0) ) * boneWeights.z;
    position2 += mul( boneMatrices[ boneIndex.w ], float4(pos, 1.0) ) * boneWeights.w;
    vsOut.pos = mul( localToClip, position2 );

#if !VULKAN
    vsOut.pos.y = -vsOut.pos.y;
#endif
    
    if (lightType == 2)
    {
        vsOut.pos.z = vsOut.pos.z * 0.5f + 0.5f; // -1..1 to 0..1 conversion
    }

    return vsOut;
}
