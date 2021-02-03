#include "ubo.h"

float ssao( float3x3 tangentToView, float3 originPosVS, float radius, int kernelSize, uint3 globalIdx )
{
    float occlusion = 0.0f;
    int depthWidth, depthHeight;
    normalTex.GetDimensions( depthWidth, depthHeight );

    for (int i = 0; i < kernelSize; ++i)
    {
        float3 samplePosVS = mul( tangentToView, kernelOffsets[ i ].xyz );
        samplePosVS = samplePosVS * radius + originPosVS;

        // project sample position:
        float4 offset = mul( viewToClip, float4( samplePosVS, 1.0f ) );
        offset.xy /= offset.w; // only need xy
        offset.xy = offset.xy * 0.5f + 0.5f; // scale/bias to texcoords
#if !VULKAN

#else
        offset.y = 1 - offset.y;
#endif
        float sampleDepth = -normalTex.Load( int3( offset.xy * float2( depthWidth, depthHeight ), 0 ) ).r;

        float diff = abs( originPosVS.z - sampleDepth );
        if (diff < 0.0001f)
        {
            occlusion += sampleDepth < samplePosVS.z ? 1.0f : 0.0f;
            continue;
        }
        
        float rangeCheck = smoothstep( 0.0f, 1.0f, radius / diff );
        occlusion += (sampleDepth < (samplePosVS.z - 0.025f) ? 1.0f : 0.0f) * rangeCheck;
    }

    occlusion = 1.0f - (occlusion / float( kernelSize ));
    float uPower = 1.8f;
    //return pow( abs( occlusion ), uPower );
    return pow( max( abs( occlusion ), 0.3f ), uPower );
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    int depthWidth, depthHeight;
    normalTex.GetDimensions( depthWidth, depthHeight );

    int noiseWidth, noiseHeight;
    specularTex.GetDimensions( noiseWidth, noiseHeight );

    //float2 noiseTexCoords = float2( float( depthWidth ) / float( noiseWidth ), float( depthHeight ) / float( noiseHeight ) );

    float2 uv = (float2( globalIdx.xy ) + 0.5f) / float2( depthWidth, depthHeight );
    // get view space origin:
    float originDepth = -normalTex.Load( int3( globalIdx.xy, 0 ) ).r;
    float uTanHalfFov = tan( cameraParams.x * 0.5f );
    float uAspectRatio = depthWidth / (float)depthHeight;
    float2 xy = uv * 2 - 1;
    //noiseTexCoords *= uv;

    float3 viewDirection = float3( -xy.x * uTanHalfFov * uAspectRatio, -xy.y * uTanHalfFov, 1.0f );
    float3 originPosVS = viewDirection * originDepth;

    // get view space normal:
    float3 normal = -normalize( normalTex.Load( int3( globalIdx.xy, 0 ) ).gba );

    // construct kernel basis matrix:
    float3 rvec = normalize( specularTex.Load( int3( globalIdx.x % noiseWidth, globalIdx.y % noiseHeight, 0 ) ).rgb );
    //float3 rvec = normalize( specularTex.Sample( sampler1, noiseTexCoords ).rgb );
    float3 tangent = normalize( rvec - normal * dot( rvec, normal ) );
    float3 bitangent = cross( tangent, normal );
    float3x3 tangentToView = transpose( float3x3( tangent, bitangent, normal ) );

    float s = ssao( tangentToView, originPosVS, 0.5f, 16, globalIdx );
    float4 color = float4( 1, 1, 1, 1 );

    rwTexture[ globalIdx.xy ] = color * s;
}
