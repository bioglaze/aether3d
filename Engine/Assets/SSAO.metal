#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

kernel void ssao( texture2d<float, access::read> colorTexture [[texture(0)]],
                  texture2d<float, access::read> depthNormalsTexture [[texture(1)]],
                  texture2d<float, access::write> outTexture [[texture(2)]],
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    const float4 color = colorTexture.read( gid );
    float4 depthNormals = depthNormalsTexture.read( gid );
    
    if (depthNormals.x >= 1)
    {
        outTexture.write( float4( 1, 1, 1, 1 ), gid.xy / 2 );
        return;
    }
    
    int num = uniforms.kernelSize;
    
    for (int i = 0; i < uniforms.kernelSize; ++i)
    {
        
    }
    
    float ao = float( num ) / float( uniforms.kernelSize );
    
    outTexture.write( color * ao, gid.xy / 2 );
}
