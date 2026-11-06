#include "ubo.h"

float ssao( float3x3 tangentToView, float3 originPosVS, float radius, int kernelSize, uint3 globalIdx, float2 depthTexDim )
{
    float occlusion = 0.0f;

    for (int i = 0; i < kernelSize; ++i)
    {
        float3 samplePosVS = mul( tangentToView, kernelOffsets[ i ].xyz );
        samplePosVS = samplePosVS * radius + originPosVS;

        // project sample position:
        float4 offset = mul( viewToClip, float4( samplePosVS, 1.0f ) );
        if (offset.w <= 1e-5f) // Sample is behind camera
        {
            //continue;
        }
        offset.xy /= offset.w; // only need xy
        offset.xy = offset.xy * 0.5f + 0.5f; // scale/bias to texcoords
#if !VULKAN

#else
        offset.y = 1 - offset.y;
#endif
        int2 texel = int2( offset.xy * depthTexDim );
        if (any( texel < 0 ) || any( texel >= int2( depthTexDim ) ) )
        {
            continue;
        }
        float sampleDepth = -normalTex.Load( int3( offset.xy * depthTexDim, 0 ) ).r;

        float diff = abs( originPosVS.z - sampleDepth );
        /*if (diff < 0.0001f)
        {
            occlusion += (sampleDepth < (samplePosVS.z - 0.025f) ? 1.0f : 0.0f) * rangeCheck;
            continue;
        }*/
        
        float rangeCheck = smoothstep( 0.0f, 1.0f, radius / diff );
        occlusion += (sampleDepth < (samplePosVS.z - 0.025f) ? 1.0f : 0.0f) * rangeCheck;
    }

    occlusion = 1.0f - (occlusion / float( kernelSize ));
    float uPower = 1.8f;

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
    float3 normalSample = normalTex.Load( int3( globalIdx.xy, 0 ) ).gba;
    // protect against zero-length normal (pixels without geometry in the depthnormals texture, for example sky)

    if (dot( normalSample, normalSample ) < 1e-8f)
    {
        rwTexture[ globalIdx.xy ] = 1.0f;
        return;
    }
    
    float3 normal = -normalize( normalSample );

    // construct kernel basis matrix:
    float3 rvec = specularTex.Load( int3( globalIdx.x % noiseWidth, globalIdx.y % noiseHeight, 0 ) ).rgb;
    float rvecLen = length( rvec );
    rvec = rvecLen > 1e-5f ? rvec / rvecLen : float3( 1, 0, 0 );
    
    float3 tangent = rvec - normal * dot( rvec, normal );
    float tangentLen = length( tangent );
    if (tangentLen < 1e-4f)
    {
        tangent = normalize( cross( normal, abs( normal.z ) < 0.999f ? float3( 0, 0, 1 ) : float3( 0, 1, 0 ) ) );
    }
    else
    {
        tangent /= tangentLen;
    }
    
    float3 bitangent = cross( tangent, normal );
    float3x3 tangentToView = transpose( float3x3( tangent, bitangent, normal ) );

    float s = ssao( tangentToView, originPosVS, 0.5f, 16, globalIdx, float2( depthWidth, depthHeight ) );
    float4 color = float4( 1, 1, 1, 1 );

    rwTexture[ globalIdx.xy ] = color * s;
}
