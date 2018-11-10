#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

float Gaussian( float x, float d )
{
    return 1.0f / (sqrt(2.0f * 3.14159f * d * d)) * exp( -(x * x) / (2.0f * d * d));
}

kernel void bloom(texture2d<float, access::read> colorTexture [[texture(0)]],
                  texture2d<float, access::write> bloomTexture [[texture(1)]],
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    //threadgroup uint ldsLightIdx[ 300 ];

    //const float4 color = colorTexture.read( gid.xy );
    //const float luminance = dot( color.rgb, float3( 0.2126f, 0.7152f, 0.0722f ) );
    const float bloomThreshold = 0.8f;

    float4 accumColor = float4( 0, 0, 0, 0 );
    
    constexpr float weights[ 9 ] = { 0.000229f, 0.005977f, 0.060598f,
                                     0.241732f, 0.382928f, 0.241732f,
                                     0.060598f, 0.005977f, 0.000229f };
    
    const float luminanceThreshold = 0.8f;
    
    // Blur
    for (int x = 0; x < 9; ++x)
    {
        const float4 color = colorTexture.read( gid.xy + ushort2( x, 0 ) );
        const float luminance = dot( color.rgb, float3( 0.2126f, 0.7152f, 0.0722f ) );
        
        accumColor += luminance > luminanceThreshold ? (color * weights[ x ]) : float4( 0, 0, 0, 0 );
    }
    
    //accumColor /= 9.0f;

    bloomTexture.write( accumColor, gid.xy );
}
