#include <metal_stdlib>
#include <metal_atomic>
#include <simd/simd.h>

using namespace metal;
#include "MetalCommon.h"

#define TILE_RES 32
#define MAX_NUM_PARTICLES_PER_TILE 1000
#define PARTICLE_INDEX_BUFFER_SENTINEL 0x7fffffff
#define NUM_THREADS_PER_TILE (TILE_RES * TILE_RES)

// Must match Renderer.hpp.
struct Particle
{
    float4 positionAndSize;
    float4 color;
    float4 clipPosition;
    float4 lifeTimeSecs;
};

float rand_1_05( float2 uv )
{
    float2 nois = (fract( sin( dot( uv, float2( 12.9898, 78.233 ) * 2.0 ) ) * 43758.5453 ) );
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
    float4 position = float4( x * 8 * sin( gid.x * 20 + uniforms.timeStamp ), gid.x % 20, x * 8 * cos( gid.x * 20 + uniforms.timeStamp ), 1 );
    position = uniforms.localToWorld * position;
    
    float4 clipPos = uniforms.viewToClip * position;
    clipPos.y = - clipPos.y;
    float3 ndc = clipPos.xyz / clipPos.w;
    float3 unscaledWindowCoords = 0.5f * ndc + float3( 0.5f, 0.5f, 0.5f );
    float3 windowCoords = float3( uniforms.windowWidth * unscaledWindowCoords.x, uniforms.windowHeight * unscaledWindowCoords.y, unscaledWindowCoords.z );

    particleBufferOut[ gid.x ].positionAndSize = position;
    particleBufferOut[ gid.x ].positionAndSize.w = 5;
    particleBufferOut[ gid.x ].color = uniforms.particleColor;
    particleBufferOut[ gid.x ].clipPosition = float4( windowCoords.x, windowCoords.y, clipPos.z, clipPos.w );
    particleBufferOut[ gid.x ].lifeTimeSecs = float4( 0, 0, 0, 0 );
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

    for (uint i = 0; i < (uint)uniforms.particleCount; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;
        const float sizePad = particles[ il ].positionAndSize.w * 2;
        
        if (il < (uint)uniforms.particleCount &&
            particles[ il ].clipPosition.x > globalIdx.x - sizePad - TILE_RES && particles[ il ].clipPosition.x < globalIdx.x + TILE_RES + sizePad &&
            particles[ il ].clipPosition.y > globalIdx.y - sizePad - TILE_RES && particles[ il ].clipPosition.y < globalIdx.y + TILE_RES + sizePad)
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

uint GetTileIndex( float2 screenPos, uint windowWidth )
{
    const float tileRes = (float)TILE_RES;
    uint numCellsX = (windowWidth + TILE_RES - 1) / TILE_RES;
    uint tileIdx = floor( screenPos.x / tileRes ) + floor( screenPos.y / tileRes ) * numCellsX;
    return tileIdx;
}

kernel void particle_draw(
                  constant Uniforms& uniforms [[ buffer(0) ]],
                  device Particle* particles [[ buffer(1) ]],
                  device uint* perTileParticleIndexBuffer [[ buffer(2) ]],
                  texture2d<float, access::read> inTexture [[texture(0)]],
                  texture2d<float, access::write> outTexture [[texture(1)]],
                  texture2d<float, access::read> inTextureDepth [[texture(2)]],
                  ushort2 gid [[thread_position_in_grid]],
                  ushort2 tid [[thread_position_in_threadgroup]],
                  ushort2 dtid [[threadgroup_position_in_grid]])
{
    const int tileIndex = GetTileIndex( float2( gid.xy ), uniforms.windowWidth );
    int index = MAX_NUM_PARTICLES_PER_TILE * tileIndex;
    int nextParticleIndex = perTileParticleIndexBuffer[ index ];

    float4 color = inTexture.read( gid );
    float depth = inTextureDepth.read( gid ).r;
    
    while (nextParticleIndex != PARTICLE_INDEX_BUFFER_SENTINEL)
    {
        uint particleIndex = nextParticleIndex;
        ++index;
        nextParticleIndex = perTileParticleIndexBuffer[ index ];

        float dist = distance( particles[ particleIndex ].clipPosition.xy, (float2)gid.xy );
        const float radius = particles[ particleIndex ].positionAndSize.w;
        if (dist < radius && particles[ particleIndex ].clipPosition.z / particles[ particleIndex ].clipPosition.w < depth)
        {
            color = particles[ particleIndex ].color;
        }
    }

    outTexture.write( color, gid );
}
