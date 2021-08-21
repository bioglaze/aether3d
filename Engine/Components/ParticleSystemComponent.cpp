#include "ParticleSystemComponent.hpp"
#include "Array.hpp"
#include "ComputeShader.hpp"
#include "System.hpp"
#include "Vec3.hpp"

Array< ae3d::ParticleSystemComponent > particleSystemComponents;
unsigned nextFreeParticleSystemComponent = 0;

unsigned ae3d::ParticleSystemComponent::New()
{
    if (nextFreeParticleSystemComponent == particleSystemComponents.count)
    {
        particleSystemComponents.Add( {} );
    }
    
    return nextFreeParticleSystemComponent++;
}

ae3d::ParticleSystemComponent* ae3d::ParticleSystemComponent::Get( unsigned index )
{
    return &particleSystemComponents[ index ];
}

void ae3d::ParticleSystemComponent::Simulate( ComputeShader& simulationShader )
{
    simulationShader.Begin();

#if RENDERER_D3D12
    //simulationShader.SetUAV( 0, particleBuffer, particleBufferDesc );
#endif
#if RENDERER_VULKAN
    //simulationShader.SetBuffer( 0, particleBufferView );
#endif

    simulationShader.Dispatch( 1, 1, 1, "Particle Simulation" );
    simulationShader.End();
}
