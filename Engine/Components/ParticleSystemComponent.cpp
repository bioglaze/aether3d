#include "ParticleSystemComponent.hpp"
#include "Array.hpp"

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

void ae3d::ParticleSystemComponent::Simulate()
{

}
