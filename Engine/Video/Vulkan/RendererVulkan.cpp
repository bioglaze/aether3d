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
    extern unsigned backBufferWidth;
    extern unsigned backBufferHeight;
}

ae3d::Renderer renderer;

VkBuffer particleBuffer;
VkDeviceMemory particleMemory;

VkBuffer particleTileBuffer;
VkDeviceMemory particleTileMemory;

void CreateBuffer( VkBuffer& buffer, int bufferSize, VkDeviceMemory& memory, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, const char* debugName );

static unsigned GetNumTilesX()
{
    return (unsigned)((GfxDeviceGlobal::backBufferWidth + 16 - 1) / (float)16);
}

static unsigned GetNumTilesY()
{
    return (unsigned)((GfxDeviceGlobal::backBufferHeight + 16 - 1) / (float)16);
}

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
    particleSimulationShader.LoadSPIRV( FileSystem::FileContents( "shaders/particle.spv" ) );
    particleDrawShader.LoadSPIRV( FileSystem::FileContents( "shaders/particle_draw.spv" ) );

    CreateBuffer( particleBuffer, 1000000 * sizeof( Particle ), particleMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "particle buffer" );
    const unsigned particleTileCount = GetNumTilesX() * GetNumTilesY();
    const unsigned maxParticlesPerTile = 1000;
    CreateBuffer( particleTileBuffer, maxParticlesPerTile * particleTileCount * sizeof( unsigned ), particleTileMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "particle tile buffer" );
}
