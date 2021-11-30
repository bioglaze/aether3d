#include "Renderer.hpp"
#include "FileSystem.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include <vulkan/vulkan.h>
#include "VulkanUtils.hpp"

namespace GfxDeviceGlobal
{
    extern VkDevice device;
}

ae3d::Renderer renderer;

VkBuffer particleBuffer;
VkDeviceMemory particleMemory;

VkBuffer particleTileBuffer;
VkBufferView particleTileBufferView;
VkDeviceMemory particleTileMemory;

void CreateBuffer( VkBuffer& buffer, int bufferSize, VkDeviceMemory& memory, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, const char* debugName );

void ae3d::BuiltinShaders::Load()
{
    spriteRendererShader.LoadSPIRV( FileSystem::FileContents( "shaders/sprite_vert.spv" ), FileSystem::FileContents( "shaders/sprite_frag.spv" ) );
    sdfShader.LoadSPIRV( FileSystem::FileContents( "shaders/sprite_vert.spv" ), FileSystem::FileContents( "shaders/sprite_frag.spv" ) );
    skyboxShader.LoadSPIRV( FileSystem::FileContents( "shaders/skybox_vert.spv" ), FileSystem::FileContents( "shaders/skybox_frag.spv" ) );
    momentsShader.LoadSPIRV( FileSystem::FileContents( "shaders/moments_vert.spv" ), FileSystem::FileContents( "shaders/moments_frag.spv" ) );
    momentsSkinShader.LoadSPIRV( FileSystem::FileContents( "shaders/moments_skin_vert.spv" ), FileSystem::FileContents( "shaders/moments_frag.spv" ) );
    depthNormalsShader.LoadSPIRV( FileSystem::FileContents( "shaders/depthnormals_vert.spv" ), FileSystem::FileContents( "shaders/depthnormals_frag.spv" ) );
    depthNormalsSkinShader.LoadSPIRV( FileSystem::FileContents( "shaders/depthnormals_skin_vert.spv" ), FileSystem::FileContents( "shaders/depthnormals_frag.spv" ) );
    uiShader.LoadSPIRV( FileSystem::FileContents( "shaders/sprite_vert.spv" ), FileSystem::FileContents( "shaders/sprite_frag.spv" ) );
    lightCullShader.LoadSPIRV( FileSystem::FileContents( "shaders/LightCuller.spv" ) );
    particleSimulationShader.LoadSPIRV( FileSystem::FileContents( "shaders/particle_simulate.spv" ) );
    particleCullShader.LoadSPIRV( FileSystem::FileContents( "shaders/particle_cull.spv" ) );
    particleDrawShader.LoadSPIRV( FileSystem::FileContents( "shaders/particle_draw.spv" ) );

    CreateBuffer( particleBuffer, 1000000 * sizeof( Particle ), particleMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "particle buffer" );
    const unsigned particleTileCount = renderer.GetNumParticleTilesX() * renderer.GetNumParticleTilesY();
    const unsigned maxParticlesPerTile = 1000;
    CreateBuffer( particleTileBuffer, maxParticlesPerTile * particleTileCount * sizeof( unsigned ), particleTileMemory, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "particle tile buffer" );

    VkBufferViewCreateInfo bufferViewInfo = {};
    bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    bufferViewInfo.flags = 0;
    bufferViewInfo.buffer = particleTileBuffer;
    bufferViewInfo.range = VK_WHOLE_SIZE;
    bufferViewInfo.format = VK_FORMAT_R32_UINT;

    VkResult err = vkCreateBufferView( GfxDeviceGlobal::device, &bufferViewInfo, nullptr, &particleTileBufferView );
    AE3D_CHECK_VULKAN( err, "particle tile buffer view" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)particleTileBufferView, VK_OBJECT_TYPE_BUFFER_VIEW, "particleTileBufferView" );

}
