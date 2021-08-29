#include "Renderer.hpp"
#include "FileSystem.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "Vec3.hpp"
#include <vulkan/vulkan.h>
#include "VulkanUtils.hpp"

struct Particle
{
    ae3d::Vec4 position;
};

namespace GfxDeviceGlobal
{
    extern VkDevice device;
}

ae3d::Renderer renderer;

VkBuffer particleBuffer;
VkDeviceMemory particleMemory;

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
    particleSimulationShader.LoadSPIRV( FileSystem::FileContents( "shaders/particle.spv" ) );

    CreateBuffer( particleBuffer, 1000000 * sizeof( Particle ), particleMemory, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, "particle buffer" );
}
