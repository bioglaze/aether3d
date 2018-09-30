#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

kernel void bloom(texture2d<float, access::read> colorTexture [[texture(0)]],
                         constant Uniforms& uniforms [[ buffer(0) ]],
                         ushort2 tid [[thread_position_in_threadgroup]],
                         ushort2 dtid [[threadgroup_position_in_grid]])
{
    threadgroup uint ldsLightIdx[ 300 ];
    threadgroup atomic_uint ldsLightIdxCounter;

    ushort2 localIdx = tid;
    ushort2 groupIdx = dtid;
}
