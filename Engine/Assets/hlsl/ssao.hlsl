#if !VULKAN
#define layout(a,b)  
#else
#define register(a) blank
#endif

#include "ubo.h"

#if 0
float ssao( int sampleCount, uint2 globalIdx )
{
    float fdepth = normalTex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) ).x;
    
    if (fdepth >= 1)
    {
        return 0.5;
    }

    int depthWidth, depthHeight;
    normalTex.GetDimensions( depthWidth, depthHeight );

    float2 uv = float2( globalIdx.x, globalIdx.y ) / float2( depthWidth, depthHeight ) * 2 - 1;
    float4 projWorldPos = mul( clipToView, float4( uv, fdepth * 2 - 1, 1 ) );
    float3 worldPos = projWorldPos.xyz / projWorldPos.w;

    int num = sampleCount;
    
    for (int i = 0; i < sampleCount; ++i )
    {
        float3 p = worldPos + kernelOffsets[ i ] * 10;
        float4 proj = mul( viewToClip, float4( p, 1 ) );
        proj.xy /= proj.w;
        proj.z = (proj.z - 0.005f) / proj.w;
        proj.xyz = proj.xyz * 0.5f + 0.5f;
        float depth = normalTex.Load( uint3( proj.xy * float2( depthWidth, depthHeight ), 0 ) ).x;

        if (depth < proj.z)
        {
            --num;
        }
    }

    float ao = float( num ) / float( sampleCount );
    return ao;
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    float4 color = tex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) ) * ssao( 64, globalIdx.xy );

    rwTexture[ globalIdx.xy ] = color;
}

#else

float ssao( float3x3 kernelBasis, float3 originPos, float radius, int kernelSize )
{
    float occlusion = 0.0f;

    for (int i = 0; i < kernelSize; ++i)
    {
        float3 samplePos = mul( kernelBasis, kernelOffsets[ i ].xyz );
        samplePos = samplePos * radius + originPos;

        // project sample position:
        float4 offset = mul( viewToClip, float4( samplePos, 1.0f ) );
        offset.xy /= offset.w; // only need xy
        offset.xy = offset.xy * 0.5f + 0.5f; // scale/bias to texcoords
        offset.y = 1 - offset.y;

        float sampleDepth = -normalTex.Sample( sLinear, offset.xy ).r;

        float rangeCheck = smoothstep( 0.0f, 1.0f, radius / abs( originPos.z - sampleDepth ) );
        occlusion += rangeCheck * step( sampleDepth, samplePos.z );
    }

    occlusion = 1.0f - (occlusion / float( kernelSize ));
    float uPower = 1.8f;
    return pow( abs( occlusion ), uPower );
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    int depthWidth, depthHeight;
    normalTex.GetDimensions( depthWidth, depthHeight );

    int noiseWidth, noiseHeight;
    specularTex.GetDimensions( noiseWidth, noiseHeight );

    float2 noiseTexCoords = float2( float( depthWidth ) / float( noiseWidth ), float( depthHeight ) / float( noiseHeight ) );

    float2 uv = float2( globalIdx.xy ) / float2( depthWidth, depthHeight );
    // get view space origin:
    float originDepth = -normalTex.Sample( sLinear, uv ).r;
    float uTanHalfFov = tan( 45.0f * 0.5f * (3.14159f / 180.0f) );
    float uAspectRatio = depthWidth / (float)depthHeight;
    float2 xy = uv * 2 - 1;
    noiseTexCoords *= uv;

    float3 viewDirection = float3( -xy.x * uTanHalfFov * uAspectRatio, -xy.y * uTanHalfFov, 1.0 );
    float3 originPos = viewDirection * originDepth;

    // get view space normal:
    float3 normal = -normalize( normalTex.Sample( sLinear, uv ).gba );

    // construct kernel basis matrix:
    //float3 rvec = normalize( specularTex.Sample( sampler1, noiseTexCoords ).rgb );
    float3 rvec = float3( 0, 0, 1 );
    float3 tangent = normalize( rvec - normal * dot( rvec, normal ) );
    float3 bitangent = cross( tangent, normal );
    float3x3 kernelBasis = float3x3( tangent, bitangent, normal );

    float s = ssao( kernelBasis, originPos, 2.2f, 32 );
    float4 color = tex.Load( uint3( globalIdx.x, globalIdx.y, 0 ) );

    rwTexture[ globalIdx.xy ] = color * s;
}
#endif
