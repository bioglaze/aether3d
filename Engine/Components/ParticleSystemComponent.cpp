#include "ParticleSystemComponent.hpp"
#include "Array.hpp"
#include "ComputeShader.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Vec3.hpp"

#ifdef RENDERER_D3D12
namespace GfxDeviceGlobal
{
    extern ID3D12Resource* particleBuffer;
    extern D3D12_UNORDERED_ACCESS_VIEW_DESC uav2Desc;
    extern ID3D12GraphicsCommandList* graphicsCommandList;
}
#endif

#if RENDERER_METAL
extern id< MTLBuffer > particleBuffer;
#endif

#if RENDERER_VULKAN
namespace GfxDeviceGlobal
{
    extern VkCommandBuffer computeCmdBuffer;
}
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
    simulationShader.SetUniformBuffer( 1, particleBuffer );
#endif
    simulationShader.Dispatch( 1, 1, 1, "Particle Simulation" );
    simulationShader.End();
}

void ae3d::ParticleSystemComponent::Draw( ComputeShader& drawShader, RenderTexture& target )
{
    drawShader.Begin();
#if RENDERER_D3D12
    D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrierDesc.UAV.pResource = target.GetGpuResource()->resource;

    GfxDeviceGlobal::graphicsCommandList->ResourceBarrier( 1, &barrierDesc );

    drawShader.SetUAV( 1, target.GetGpuResource()->resource, *target.GetUAVDesc() );
    drawShader.SetUAV( 2, GfxDeviceGlobal::particleBuffer, GfxDeviceGlobal::uav2Desc );
#endif
#if RENDERER_VULKAN
    target.SetColorImageLayout( VK_IMAGE_LAYOUT_GENERAL, GfxDeviceGlobal::computeCmdBuffer );
    drawShader.SetRenderTexture( &target, 14 );
#else
    drawShader.SetRenderTexture( &target, 0 );
#endif
#if RENDERER_METAL
    drawShader.SetUniformBuffer( 1, particleBuffer );
    drawShader.SetRenderTexture( &target, 1 );
#endif

    drawShader.Dispatch( target.GetWidth() / 8, target.GetHeight() / 8, 1, "Particle Draw" );

#if RENDERER_VULKAN
    //target.SetColorImageLayout( VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, GfxDeviceGlobal::computeCmdBuffer );
    target.SetColorImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, GfxDeviceGlobal::computeCmdBuffer );
#endif

    drawShader.End();
}
