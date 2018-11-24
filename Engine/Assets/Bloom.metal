#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

float Gaussian( float x, float d )
{
    return 1.0f / (sqrt(2.0f * 3.14159f * d * d)) * exp( -(x * x) / (2.0f * d * d));
}

kernel void downsampleAndThreshold(texture2d<float, access::read> colorTexture [[texture(0)]],
                  texture2d<float, access::write> downsampledBrightTexture [[texture(1)]],
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    const float4 color = colorTexture.read( gid );
    const float luminance = dot( color.rgb, float3( 0.2126f, 0.7152f, 0.0722f ) );
    const float luminanceThreshold = 0.7f;
    const float4 finalColor = luminance > luminanceThreshold ? color : float4( 0, 0, 0, 0 );
    downsampledBrightTexture.write( finalColor, gid.xy / 2 );
}

kernel void blur(texture2d<float, access::read> inputTexture [[texture(0)]],
                  texture2d<float, access::write> resultTexture [[texture(1)]],
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    float4 accumColor = float4( 0, 0, 0, 0 );
    
    // Box
    const int radius = 13;
    for (int x = 0; x < radius; ++x)
    {
        const float4 color = inputTexture.read( gid + ushort2( x * uniforms.tilesXY.z - radius / 2, x * uniforms.tilesXY.w - radius / 2 ) );
        accumColor += color;
    }
    
    accumColor /= radius;

    // Gaussian
    /*constexpr float weights[ 9 ] = { 0.000229f, 0.005977f, 0.060598f,
        0.241732f, 0.382928f, 0.241732f,
        0.060598f, 0.005977f, 0.000229f };

    for (int x = 0; x < 9; ++x)
    {
        const float4 color = inputTexture.read( gid + ushort2( x * uniforms.tilesXY.z - 5, x * uniforms.tilesXY.w - 5 ) ) * weights[ x ];
        accumColor += color;
    }*/
    
    resultTexture.write( float4(accumColor.rgb, 1), gid.xy );
}
