#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

struct Particle
{
    float4 position;
};

kernel void particle_simulation(
                  device Particle* particleBufferOut [[ buffer(2) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    Particle p;
    p.position = float4( 10, 0, 0, 1 );
    particleBufferOut[ gid.x ] = p;
}
