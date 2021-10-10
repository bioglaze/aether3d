#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

struct Particle
{
    float4 position;
    float4 color;
    float4 clipPosition;
};

kernel void particle_simulation(
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  device Particle* particleBufferOut [[ buffer(1) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    Particle p;
    p.position = float4( 0, 0, 0, 1 );
    p.clipPosition = uniforms.viewToClip * p.position;
    p.color = float4( 1, 0, 0, 1 );
    particleBufferOut[ gid.x ] = p;
}

kernel void particle_draw(
                  device Particle* particleBuffer [[ buffer(1) ]],
                  texture2d<float, access::read_write> outTexture [[texture(0)]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    float4 color = outTexture.read( gid );
    
    for (uint i = 0; i < 10; ++i)
    {
        if (particleBuffer[ i ].clipPosition.x == gid.x && particleBuffer[ i ].clipPosition.y == gid.y)
        {
            color += particleBuffer[ i ].color;
        }
    }

    outTexture.write( color, gid );
}
