#include "LightTiler.hpp"
#include "System.hpp"
#include "GfxDevice.hpp"
#include "VulkanUtils.hpp"
// TODO: Some of these methods can be shared with D3D12 and possibly Metal.

namespace MathUtil
{
    int Max( int x, int y );
}

namespace GfxDeviceGlobal
{
    extern int backBufferWidth;
    extern int backBufferHeight;
    extern VkDevice device;
}

namespace ae3d
{
    void GetMemoryType( std::uint32_t typeBits, VkFlags properties, std::uint32_t* typeIndex );
}

unsigned ae3d::LightTiler::GetMaxNumLightsPerTile() const
{
    const unsigned kAdjustmentMultipier = 32;

    // I haven't tested at greater than 1080p, so cap it
    const unsigned uHeight = (GfxDeviceGlobal::backBufferHeight > 1080) ? 1080 : GfxDeviceGlobal::backBufferHeight;

    // adjust max lights per tile down as height increases
    return (MaxLightsPerTile - (kAdjustmentMultipier * (uHeight / 120)));
}

void ae3d::LightTiler::Init()
{
    pointLightCenterAndRadius.resize( MaxLights );
    spotLightCenterAndRadius.resize( MaxLights );

    // Point light buffer
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = pointLightCenterAndRadius.size() * 4 * sizeof( float );
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &pointLightCenterAndRadiusBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightCenterAndRadiusBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "pointLightCenterAndRadiusBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, pointLightCenterAndRadiusBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memReqs.size;
        GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &pointLightCenterAndRadiusMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory pointLightCenterAndRadiusMemory" );

        err = vkBindBufferMemory( GfxDeviceGlobal::device, pointLightCenterAndRadiusBuffer, pointLightCenterAndRadiusMemory, 0 );
        AE3D_CHECK_VULKAN( err, "vkBindBufferMemory pointLightCenterAndRadiusBuffer" );

        pointLightDesc.buffer = pointLightCenterAndRadiusBuffer;
        pointLightDesc.offset = 0;
        pointLightDesc.range = bufferInfo.size;
    }
}

void ae3d::LightTiler::SetPointLightPositionAndRadius( int bufferIndex, Vec3& position, float radius )
{
    System::Assert( bufferIndex < MaxLights, "tried to set a too high light index" );

    if (bufferIndex < MaxLights)
    {
        activePointLights = MathUtil::Max( bufferIndex + 1, activePointLights );
        pointLightCenterAndRadius[ bufferIndex ] = Vec4( position.x, position.y, position.z, radius );
    }
}

void ae3d::LightTiler::SetSpotLightPositionAndRadius( int bufferIndex, Vec3& position, float radius )
{
    System::Assert( bufferIndex < MaxLights, "tried to set a too high light index" );

    if (bufferIndex < MaxLights)
    {
        activeSpotLights = MathUtil::Max( bufferIndex + 1, activeSpotLights );
        spotLightCenterAndRadius[ bufferIndex ] = Vec4( position.x, position.y, position.z, radius );
    }
}
