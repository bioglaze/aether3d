#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

float ssao_internal( float3x3 kernelBasis, float3 originPos, float radius, int kernelSize, ushort2 globalIdx, constant Uniforms& uniforms,
                    texture2d<float, access::read> depthNormalsTexture )
{
    float occlusion = 0.0f;

    for (int i = 0; i < kernelSize; ++i)
    {
        float3 samplePos = kernelBasis * uniforms.kernelOffsets[ i ].xyz;
        samplePos = samplePos * radius + originPos;

        // project sample position:
        float4 offset = uniforms.viewToClip * float4( samplePos, 1.0f );
        offset.xy /= offset.w; // only need xy
        offset.xy = offset.xy * 0.5f + 0.5f; // scale/bias to texcoords

        int depthWidth = uniforms.windowWidth, depthHeight = uniforms.windowHeight;
        float sampleDepth = -depthNormalsTexture.read( uint2( offset.xy * float2( depthWidth, depthHeight ) ), 0 ).r;

        const float diff = abs( originPos.z - sampleDepth );
        if (diff < 0.0001f)
        {
            occlusion += (sampleDepth < (samplePos.z - 0.025f) ? 1.0f : 0.0f);
            continue;
        }
        
        float rangeCheck = smoothstep( 0.0f, 1.0f, radius / diff );
        occlusion += (sampleDepth < (samplePos.z - 0.025f) ? 1.0f : 0.0f) * rangeCheck;
    }

    occlusion = 1.0f - (occlusion / float( kernelSize ));
    float uPower = 1.8f;
    return pow( abs( occlusion ), uPower );
}

kernel void ssao( texture2d<float, access::read> colorTexture [[texture(0)]],
                  texture2d<float, access::read> depthNormalsTexture [[texture(1)]],
                  texture2d<float, access::write> outTexture [[texture(2)]],
                  texture2d<float, access::read> noiseTexture [[texture(3)]],
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  ushort2 globalIdx [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    const float4 color = colorTexture.read( globalIdx );
    
    float depthWidth = uniforms.windowWidth;
    float depthHeight = uniforms.windowHeight;
    float noiseWidth = 64;
    float noiseHeight = 64;
    float2 noiseTexCoords = float2( float( depthWidth ) / float( noiseWidth ), float( depthHeight ) / float( noiseHeight ) );

    float2 uv = float2( globalIdx.xy ) / float2( depthWidth, depthHeight );
    // get view space origin:
    float originDepth = -depthNormalsTexture.read( uint2( globalIdx.xy ), 0 ).r;
    float uTanHalfFov = tan( 45.0f * 0.5f * (3.14159f / 180.0f) );
    float uAspectRatio = depthWidth / (float)depthHeight;
    float2 xy = uv * 2 - 1;
    noiseTexCoords *= uv;
    
    float3 viewDirection = float3( -xy.x * uTanHalfFov * uAspectRatio, -xy.y * uTanHalfFov, 1.0 );
    float3 originPos = viewDirection * originDepth;
    
    // get view space normal:
    float3 normal = -normalize( depthNormalsTexture.read( uint2( globalIdx.xy ), 0 ).gba );
    
    // construct kernel basis matrix:
    float3 rvec = normalize( noiseTexture.read( uint2( globalIdx.x % 64, globalIdx.y % 64 ), 0 ).rgb );
    float3 tangent = normalize( rvec - normal * dot( rvec, normal ) );
    float3 bitangent = cross( tangent, normal );
    float3x3 kernelBasis = float3x3( tangent, bitangent, normal );
    
    float s = ssao_internal( kernelBasis, originPos, 0.5f, 32, globalIdx, uniforms, depthNormalsTexture );
    
    outTexture.write( color * s, globalIdx.xy );
}
