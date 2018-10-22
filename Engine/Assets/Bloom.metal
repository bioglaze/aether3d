#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

kernel void bloom(texture2d<float, access::read> colorTexture [[texture(0)]],
                  texture2d<float, access::write> bloomTexture [[texture(1)]],
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    //threadgroup uint ldsLightIdx[ 300 ];
    
    float4 accumColor = float4( 0, 0, 0, 0 );
    
    constexpr float weights[ 9 ] = { 0.000229f, 0.005977f, 0.060598f,
                                     0.241732f, 0.382928f, 0.241732f,
                                     0.060598f, 0.005977f, 0.000229f };
    // Blur
    for (int x = 0; x < 9; ++x)
    {
        accumColor += colorTexture.read( gid.xy + ushort2( x, 0 ) ) * weights[ x ];
    }
    
    //accumColor /= 9;
    
    const float luminance = dot( accumColor.rgb, float3( 0.2126f, 0.7152f, 0.0722f ) );

    bloomTexture.write( accumColor, gid.xy );
}
