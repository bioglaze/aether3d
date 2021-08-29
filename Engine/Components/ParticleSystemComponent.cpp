#include "ParticleSystemComponent.hpp"
#include "Array.hpp"
#include "ComputeShader.hpp"
#include "System.hpp"
#include "Vec3.hpp"

#ifdef RENDERER_D3D12
namespace GfxDeviceGlobal
{
    extern ID3D12Resource* particleBuffer;
    extern D3D12_UNORDERED_ACCESS_VIEW_DESC uav2Desc;
}
#endif
#if RENDERER_METAL
extern id< MTLBuffer > particleBuffer;
#endif

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
    simulationShader.SetUAV( 2, GfxDeviceGlobal::particleBuffer, GfxDeviceGlobal::uav2Desc );
#endif
#if RENDERER_METAL
    simulationShader.SetUniformBuffer( 0, particleBuffer );
#endif
    simulationShader.Dispatch( 1, 1, 1, "Particle Simulation" );
    simulationShader.End();
}

void ae3d::ParticleSystemComponent::Draw( ComputeShader& drawShader, int width, int height )
{
    drawShader.Begin();
    drawShader.Dispatch( width / 8, height / 8, 1, "Particle Draw" );
    drawShader.End();
}
