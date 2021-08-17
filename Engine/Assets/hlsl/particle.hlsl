#include "ubo.h"

// Must be kept in sync with ParticleSystemComponent.cpp
struct Particle
{
    float4 position;
};

RWStructuredBuffer< Particle > particles;

[numthreads( 64, 1, 1 )]
void CSMain( uint3 globalIdx : SV_DispatchThreadID, uint3 localIdx : SV_GroupThreadID, uint3 groupIdx : SV_GroupID )
{
    particles[ globalIdx.x ].position = float4( 5, 0, 0, 1 );
}
