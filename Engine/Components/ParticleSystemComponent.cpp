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
    extern unsigned backBufferWidth;
    extern unsigned backBufferHeight;
}

void TransitionResource( GpuResource& gpuResource, D3D12_RESOURCE_STATES newState );
void UploadPerObjectUbo();
#endif

#if RENDERER_METAL
extern id< MTLBuffer > particleBuffer;
#endif

#if RENDERER_VULKAN
namespace GfxDeviceGlobal
{
    extern VkCommandBuffer computeCmdBuffer;
    extern unsigned backBufferWidth;
    extern unsigned backBufferHeight;
    extern PerObjectUboStruct perObjectUboStruct;
}

extern VkBuffer particleTileBuffer;

#endif

static const unsigned TileRes = 32;

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
    if (particleSystemComponents.count == 0)
    {
        return;
    }
    
    simulationShader.Begin();

#if RENDERER_D3D12
    UploadPerObjectUbo();
    simulationShader.SetUAV( 2, GfxDeviceGlobal::particleBuffer, GfxDeviceGlobal::uav2Desc );
#endif
#if RENDERER_METAL
    simulationShader.SetUniformBuffer( 1, particleBuffer );
#endif
    simulationShader.Dispatch( GfxDeviceGlobal::perObjectUboStruct.particleCount / 64, 1, 1, "Particle Simulation" );
    simulationShader.End();
}

void ae3d::ParticleSystemComponent::Cull( ComputeShader& cullShader )
{
    if (particleSystemComponents.count == 0)
    {
        return;
    }

    GfxDeviceGlobal::perObjectUboStruct.windowWidth = GfxDeviceGlobal::backBufferWidth;
    GfxDeviceGlobal::perObjectUboStruct.windowHeight = GfxDeviceGlobal::backBufferHeight;
    cullShader.Begin();

#if RENDERER_D3D12
    UploadPerObjectUbo();
    cullShader.SetUAV( 2, GfxDeviceGlobal::particleBuffer, GfxDeviceGlobal::uav2Desc );
    cullShader.SetUAV( 3, GfxDeviceGlobal::particleTileBuffer, GfxDeviceGlobal::uav3Desc );
#endif
#if RENDERER_METAL
    cullShader.SetUniformBuffer( 1, particleBuffer );
#endif
    cullShader.Dispatch( renderer.GetNumParticleTilesX(), renderer.GetNumParticleTilesY(), 1, "Particle Cull" );
    cullShader.End();
}

void ae3d::ParticleSystemComponent::Draw( ComputeShader& drawShader, RenderTexture& target )
{
    if (particleSystemComponents.count == 0)
    {
        return;
    }

    GfxDeviceGlobal::perObjectUboStruct.windowWidth = GfxDeviceGlobal::backBufferWidth;
    GfxDeviceGlobal::perObjectUboStruct.windowHeight = GfxDeviceGlobal::backBufferHeight;
    drawShader.Begin();
#if RENDERER_D3D12
    UploadPerObjectUbo();
    /*D3D12_RESOURCE_BARRIER barrierDesc = {};
    barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrierDesc.UAV.pResource = target.GetGpuResource()->resource;

    GfxDeviceGlobal::graphicsCommandList->ResourceBarrier( 1, &barrierDesc );
    
    barrierDesc.UAV.pResource = GfxDeviceGlobal::particleBuffer;
    GfxDeviceGlobal::graphicsCommandList->ResourceBarrier( 1, &barrierDesc );

    barrierDesc.UAV.pResource = GfxDeviceGlobal::particleTileBuffer;
    GfxDeviceGlobal::graphicsCommandList->ResourceBarrier( 1, &barrierDesc );*/

    TransitionResource( *target.GetGpuResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS );

    drawShader.SetUAV( 1, target.GetGpuResource()->resource, *target.GetUAVDesc() );
    drawShader.SetUAV( 2, GfxDeviceGlobal::particleBuffer, GfxDeviceGlobal::uav2Desc );
    drawShader.SetUAV( 3, GfxDeviceGlobal::particleTileBuffer, GfxDeviceGlobal::uav3Desc );
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
    drawShader.SetRenderTextureDepth( &target, 2 );
#endif

    drawShader.Dispatch( target.GetWidth() / 8, target.GetHeight() / 8, 1, "Particle Draw" );

#if RENDERER_VULKAN
    target.SetColorImageLayout( VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, GfxDeviceGlobal::computeCmdBuffer );
#endif

    drawShader.End();
}
