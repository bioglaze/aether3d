#include "ubo.h"

#define TILE_RES 32
#define MAX_NUM_PARTICLES_PER_TILE 1000
#define PARTICLE_INDEX_BUFFER_SENTINEL 0x7fffffff
#define NUM_THREADS_PER_TILE (TILE_RES * TILE_RES)

groupshared uint ldsParticleIdxCounter;
groupshared uint ldsParticlesIdx[ MAX_NUM_PARTICLES_PER_TILE ];

uint GetNumTilesX()
{
    return (uint)((windowWidth + TILE_RES - 1) / (float)TILE_RES);
}

uint GetNumTilesY()
{
    return (uint)((windowHeight + TILE_RES - 1) / (float)TILE_RES);
}

[numthreads( TILE_RES, TILE_RES, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    uint localIdxFlattened = localIdx.x + localIdx.y * TILE_RES;
    uint tileIdxFlattened = groupIdx.x + groupIdx.y * GetNumTilesX();

    if (localIdxFlattened == 0)
    {
        ldsParticleIdxCounter = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint i = 0; i < particleCount; i += NUM_THREADS_PER_TILE)
    {
        uint il = localIdxFlattened + i;
        const float sizePad = particles[ il ].positionAndSize.w * 2;

        if (il < particleCount &&
            particles[ il ].clipPosition.w != 666 && 
            particles[ il ].clipPosition.x > globalIdx.x - sizePad - TILE_RES && particles[ il ].clipPosition.x < globalIdx.x + TILE_RES + sizePad &&
            particles[ il ].clipPosition.y > globalIdx.y - sizePad - TILE_RES && particles[ il ].clipPosition.y < globalIdx.y + TILE_RES + sizePad)
        {
            uint dstIdx = 0;
            InterlockedAdd( ldsParticleIdxCounter, 1, dstIdx );
            ldsParticlesIdx[ dstIdx ] = il;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    uint startOffset = MAX_NUM_PARTICLES_PER_TILE * tileIdxFlattened;

    for (uint k = localIdxFlattened; k < ldsParticleIdxCounter; k += NUM_THREADS_PER_TILE)
    {
        perTileParticleIndexBuffer[ startOffset + k ] = ldsParticlesIdx[ k ];
    }

    if (localIdxFlattened == 0)
    {
        perTileParticleIndexBuffer[ startOffset + ldsParticleIdxCounter ] = PARTICLE_INDEX_BUFFER_SENTINEL;
    }
}
