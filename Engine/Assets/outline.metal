#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

constexpr sampler sLinear( coord::normalized, address::repeat, filter::linear );

kernel void outline(texture2d<float, access::sample> colorTexture [[texture(0)]],
                  texture2d<float, access::write> outlineTexture [[texture(1)]],
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    // Algorithm source: https://ourmachinery.com/post/borderland-part-3-selection-highlighting/
    float2 uv = (float2)gid.xy / float2( uniforms.windowWidth, uniforms.windowHeight );
    float4 id0 = colorTexture.gather( sLinear, uv, int2( -2, -2 ) );
    float4 id1 = colorTexture.gather( sLinear, uv, int2( 0, -2 ) );
    float4 id2 = colorTexture.gather( sLinear, uv, int2( -2, 0 ) );
    float4 id3 = colorTexture.gather( sLinear, uv, int2( 0, 0 ) );

    id2.xw = id1.xy;
    float idCenter = id3.w;
    id3.w = id0.y;

    const float avg_scalar = 1.f / 8.f;
    float a = dot( float4( id2 != idCenter ), avg_scalar );
    a += dot( float4( id3 != idCenter ), avg_scalar );

    outlineTexture.write( a, gid.xy );
}
