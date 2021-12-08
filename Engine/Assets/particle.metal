#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

#define TILE_RES 32
#define MAX_NUM_PARTICLES_PER_TILE 1000
#define PARTICLE_INDEX_BUFFER_SENTINEL 0x7fffffff
#define NUM_THREADS_PER_TILE (TILE_RES * TILE_RES)

struct Particle
{
    float4 position;
    float4 color;
    float4 clipPosition;
};

float rand_1_05( float2 uv )
{
    float2 nois = (fract( sin( dot( uv, float2( 12.9898, 78.233) * 2.0 ) ) * 43758.5453 ) );
    return abs( nois.x + nois.y ) * 0.5;
}

kernel void particle_simulation(
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  device Particle* particleBufferOut [[ buffer(1) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    float2 uv = (float2)gid.xy;
    float x = rand_1_05( uv );
    float4 position = float4( gid.x * x * 8, sin( 1.0f/*uniforms.timeStamp*/ ) * 8, 0, 1 );
    float4 clipPos = uniforms.viewToClip * position;
    clipPos.y = - clipPos.y;
    float3 ndc = clipPos.xyz / clipPos.w;
    float3 unscaledWindowCoords = 0.5f * ndc + float3( 0.5f, 0.5f, 0.5f );
    float3 windowCoords = float3( uniforms.windowWidth * unscaledWindowCoords.x, uniforms.windowHeight * unscaledWindowCoords.y, unscaledWindowCoords.z );

    particleBufferOut[ gid.x ].position = position;
    particleBufferOut[ gid.x ].color = float4( 1, 0, 0, 1 );
    particleBufferOut[ gid.x ].clipPosition = float4( windowCoords.x, windowCoords.y, clipPos.z, clipPos.w );
}

uint GetNumTilesX( uint windowWidth )
{
    return (uint)((windowWidth + TILE_RES - 1) / (float)TILE_RES);
}

kernel void particle_cull(
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  device Particle* particles [[ buffer(1) ]],
                  device uint* perTileParticleIndexBuffer [[ buffer(2) ]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    threadgroup atomic_uint ldsParticleIdxCounter;
    threadgroup uint ldsParticlesIdx[ MAX_NUM_PARTICLES_PER_TILE ];

    ushort2 localIdx = tid;
    ushort2 groupIdx = dtid;
    ushort2 globalIdx = gid;
    
    uint localIdxFlattened = localIdx.x + localIdx.y * TILE_RES;
    uint tileIdxFlattened = groupIdx.x + groupIdx.y * GetNumTilesX( uniforms.windowWidth );

    if (localIdxFlattened == 0)
    {
        atomic_store_explicit( &ldsParticleIdxCounter, 0, memory_order_relaxed );
    }

    threadgroup_barrier( mem_flags::mem_threadgroup );

    for (uint i = 0; i < uniforms.particleCount; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;

        if (il < uniforms.particleCount &&
            particles[ il ].clipPosition.x > globalIdx.x && particles[ il ].clipPosition.x < globalIdx.x + TILE_RES &&
            particles[ il ].clipPosition.y > globalIdx.y && particles[ il ].clipPosition.y < globalIdx.y + TILE_RES)
        {
            uint dstIdx = atomic_fetch_add_explicit( &ldsParticleIdxCounter, 1, memory_order::memory_order_relaxed );
            ldsParticlesIdx[ dstIdx ] = il;
        }
    }

    threadgroup_barrier( mem_flags::mem_threadgroup );

    uint startOffset = MAX_NUM_PARTICLES_PER_TILE * tileIdxFlattened;
    uint particleCountInTile = atomic_load_explicit( &ldsParticleIdxCounter, memory_order::memory_order_relaxed );
    
     for (uint k = localIdxFlattened; k < particleCountInTile; k += NUM_THREADS_PER_TILE)
     {
         perTileParticleIndexBuffer[ startOffset + k ] = ldsParticlesIdx[ k ];
     }

     if (localIdxFlattened == 0)
     {
         perTileParticleIndexBuffer[ startOffset + particleCountInTile ] = PARTICLE_INDEX_BUFFER_SENTINEL;
     }
}

kernel void particle_draw(
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  device Particle* particles [[ buffer(1) ]],
                  texture2d<float, access::read> inTexture [[texture(0)]],
                  texture2d<float, access::write> outTexture [[texture(1)]],
                  texture2d<float, access::read> inTextureDepth [[texture(2)]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    float4 color = inTexture.read( gid );
    float depth = inTextureDepth.read( gid ).r;
    
    for (uint i = 0; i < (uint)min( uniforms.particleCount, 300 ); ++i)
    {
        float dist = distance( particles[ i ].clipPosition.xy, (float2)gid.xy );
        const float radius = 5;
        if (dist < radius && particles[ i ].clipPosition.z / particles[ i ].clipPosition.w < depth)
        {
            color = particles[ i ].color;
        }
    }

    outTexture.write( color, gid );
}
