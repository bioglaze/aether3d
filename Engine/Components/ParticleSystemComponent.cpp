#include "ParticleSystemComponent.hpp"
#include "Array.hpp"
#include "ComputeShader.hpp"
#include "GfxDevice.hpp"
#include "RenderTexture.hpp"
#include "Renderer.hpp"
#include "System.hpp"
#include "Vec3.hpp"

extern ae3d::Renderer renderer;

#ifdef RENDERER_D3D12
namespace GfxDeviceGlobal
{
    extern ID3D12Resource* particleBuffer;
    extern ID3D12Resource* particleTileBuffer;
    extern D3D12_UNORDERED_ACCESS_VIEW_DESC uav2Desc;
    extern D3D12_UNORDERED_ACCESS_VIEW_DESC uav3Desc;
    extern ID3D12GraphicsCommandList* graphicsCommandList;
    extern PerObjectUboStruct perObjectUboStruct;
}

void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );
#endif

#if RENDERER_METAL
extern id< MTLBuffer > particleBuffer;
extern id< MTLBuffer > particleTileBuffer;

namespace GfxDeviceGlobal
{
    extern PerObjectUboStruct perObjectUboStruct;
}
#endif

#if RENDERER_VULKAN
namespace GfxDeviceGlobal
{
    extern VkCommandBuffer computeCmdBuffer;
    extern PerObjectUboStruct perObjectUboStruct;
}

extern VkBuffer particleTileBuffer;

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

bool ae3d::ParticleSystemComponent::IsAnyAlive()
{
    for (unsigned i = 0; i < particleSystemComponents.count; ++i)
    {
        if (particleSystemComponents[ i ].gameObject != nullptr)
        {
            return true;
        }
    }

    return false;
}

void ae3d::ParticleSystemComponent::SetMaxParticles( int count )
{
    if (count < 100000)
    {
        maxParticles = count;
    }
    else
    {
        System::Print( "Too many particles in SetMaxParticles! Tried to set %d, max is 100000.\n", count );
        maxParticles = 99999;
    }
}

void ae3d::ParticleSystemComponent::Simulate( ComputeShader& simulationShader )
{
    if (!ParticleSystemComponent::IsAnyAlive())
    {
        return;
    }
    
    GfxDeviceGlobal::perObjectUboStruct.particleCount = maxParticles;
    
    simulationShader.Begin();

#if RENDERER_D3D12
    simulationShader.SetUAV( 2, GfxDeviceGlobal::particleBuffer, GfxDeviceGlobal::uav2Desc );
#endif
#if RENDERER_METAL
    simulationShader.SetUniformBuffer( 1, particleBuffer );
    simulationShader.Dispatch( GfxDeviceGlobal::perObjectUboStruct.particleCount / 64, 1, 1, "Particle Simulation", 64, 1 );
#else
    simulationShader.Dispatch( GfxDeviceGlobal::perObjectUboStruct.particleCount / 64, 1, 1, "Particle Simulation" );
#endif
    simulationShader.End();
}

void ae3d::ParticleSystemComponent::Cull( ComputeShader& cullShader )
{
    if (!ParticleSystemComponent::IsAnyAlive())
    {
        return;
    }

    GfxDeviceGlobal::perObjectUboStruct.windowWidth = GfxDevice::backBufferWidth;
    GfxDeviceGlobal::perObjectUboStruct.windowHeight = GfxDevice::backBufferHeight;
    GfxDeviceGlobal::perObjectUboStruct.particleCount = maxParticles;
    cullShader.Begin();

#if RENDERER_D3D12
    cullShader.SetUAV( 2, GfxDeviceGlobal::particleBuffer, GfxDeviceGlobal::uav2Desc );
    cullShader.SetUAV( 3, GfxDeviceGlobal::particleTileBuffer, GfxDeviceGlobal::uav3Desc );
#endif
#if RENDERER_METAL
    cullShader.SetUniformBuffer( 1, particleBuffer );
    cullShader.SetUniformBuffer( 2, particleTileBuffer );
    cullShader.Dispatch( renderer.GetNumParticleTilesX(), renderer.GetNumParticleTilesY(), 1, "Particle Cull", 32, 32 );
#else
    cullShader.Dispatch( renderer.GetNumParticleTilesX(), renderer.GetNumParticleTilesY(), 1, "Particle Cull" );
#endif
    cullShader.End();
}

void ae3d::ParticleSystemComponent::Draw( ComputeShader& drawShader, RenderTexture& target )
{
    if (!ParticleSystemComponent::IsAnyAlive())
    {
        return;
    }

    GfxDeviceGlobal::perObjectUboStruct.windowWidth = GfxDevice::backBufferWidth;
    GfxDeviceGlobal::perObjectUboStruct.windowHeight = GfxDevice::backBufferHeight;
    GfxDeviceGlobal::perObjectUboStruct.particleCount = maxParticles;
    drawShader.Begin();
#if RENDERER_D3D12
    TransitionResource( *target.GetGpuResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS );

    drawShader.SetUAV( 1, target.GetGpuResource()->resource, *target.GetUAVDesc() );
    drawShader.SetUAV( 2, GfxDeviceGlobal::particleBuffer, GfxDeviceGlobal::uav2Desc );
    drawShader.SetUAV( 3, GfxDeviceGlobal::particleTileBuffer, GfxDeviceGlobal::uav3Desc );
#endif
#if RENDERER_VULKAN
    target.SetColorImageLayout( VK_IMAGE_LAYOUT_GENERAL, GfxDeviceGlobal::computeCmdBuffer );
    //target.SetDepthImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, GfxDeviceGlobal::computeCmdBuffer );
    drawShader.End(); // Fixes a synchronization validation error for target texture.
    drawShader.Begin();
    drawShader.SetRenderTexture( &target, 14 );
    //drawShader.SetRenderTextureDepth( &target, 0 );
#else
    drawShader.SetRenderTexture( &target, 0 );
#endif
#if RENDERER_METAL
    drawShader.SetUniformBuffer( 1, particleBuffer );
    drawShader.SetUniformBuffer( 2, particleTileBuffer );
    drawShader.SetRenderTexture( &target, 1 );
    drawShader.SetRenderTextureDepth( &target, 2 );
    drawShader.Dispatch( target.GetWidth() / 8, target.GetHeight() / 8, 1, "Particle Draw", 8, 8 );
#else
    drawShader.Dispatch( target.GetWidth() / 8, target.GetHeight() / 8, 1, "Particle Draw" );
#endif

#if RENDERER_VULKAN
    drawShader.End(); // Fixes a synchronization validation error for target texture.
    drawShader.Begin();
    target.SetColorImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, GfxDeviceGlobal::computeCmdBuffer );
#endif

    drawShader.End();
}
