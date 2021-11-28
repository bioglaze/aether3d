#include "ubo.h"

#define TILE_RES 16
#define PARTICLE_INDEX_BUFFER_SENTINEL 0x7fffffff
#define MAX_PARTICLES_PER_TILE 1000

uint GetTileIndex( float2 screenPos )
{
    const float tileRes = (float)TILE_RES;
    uint numCellsX = (windowWidth + TILE_RES - 1) / TILE_RES;
    uint tileIdx = floor( screenPos.x / tileRes ) + floor( screenPos.y / tileRes ) * numCellsX;
    return tileIdx;
}

[numthreads( 8, 8, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    const uint tileIndex = GetTileIndex( globalIdx.xy );
    uint index = MAX_PARTICLES_PER_TILE * tileIndex;
    uint nextParticleIndex = perTileParticleIndexBuffer[ index ];

    float4 color = rwTexture[ globalIdx.xy ];
    
    while (nextParticleIndex != PARTICLE_INDEX_BUFFER_SENTINEL)
    {
        uint particleIndex = nextParticleIndex;
        ++index;
        nextParticleIndex = perTileParticleIndexBuffer[ index ];

        float dist = distance( particles[ particleIndex ].clipPosition.xy, globalIdx.xy );
        const float radius = particles[ particleIndex ].clipPosition.z;
        
        if (dist < radius)
        {
            color = particles[ particleIndex ].color;
        }
    }

    rwTexture[ globalIdx.xy ] = color; 
}
