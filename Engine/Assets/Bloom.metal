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
    float weights[ 5 ] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
    float4 accumColor = inputTexture.read( uint2( gid.x, gid.y ) ) * weights[ 0 ];

    for (int x = 1; x < 5; ++x)
    {
        accumColor += inputTexture.read( uint2( gid.x + x * uniforms.tilesXY.z, gid.y + x * uniforms.tilesXY.w ) ) * weights[ x ];
        accumColor += inputTexture.read( uint2( gid.x - x * uniforms.tilesXY.z, gid.y - x * uniforms.tilesXY.w ) ) * weights[ x ];
    }

    resultTexture.write( accumColor, gid.xy );
}
