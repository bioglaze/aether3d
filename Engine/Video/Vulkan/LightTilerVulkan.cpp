// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "LightTiler.hpp"
#include <cstring>
#include "ComputeShader.hpp"
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "RenderTexture.hpp"
#include "Renderer.hpp"
#include "Statistics.hpp"
#include "System.hpp"
#include "VulkanUtils.hpp"

extern ae3d::Renderer renderer;

namespace MathUtil
{
    int Max( int x, int y );
}

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern PerObjectUboStruct perObjectUboStruct;
    extern VkCommandBuffer computeCmdBuffer;
    extern VkDescriptorSetLayout descriptorSetLayout;
    extern VkQueue computeQueue;
    extern VkImageView boundViews[ ae3d::ComputeShader::SLOT_COUNT ];
    extern VkSampler boundSamplers[ 2 ];
}

void UploadPerObjectUbo();

void ae3d::LightTiler::DestroyBuffers()
{
    vkDestroyBuffer( GfxDeviceGlobal::device, perTileLightIndexBuffer, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, pointLightCenterAndRadiusBuffer, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, pointLightColorBuffer, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, spotLightColorBuffer, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, spotLightCenterAndRadiusBuffer, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, spotLightParamsBuffer, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, perTileLightIndexBufferView, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, pointLightBufferView, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, pointLightColorView, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, spotLightColorView, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, spotLightBufferView, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, spotLightParamsView, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, perTileLightIndexBufferMemory, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, pointLightCenterAndRadiusMemory, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, pointLightColorMemory, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, spotLightColorMemory, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, spotLightCenterAndRadiusMemory, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, spotLightParamsMemory, nullptr );
}

void ae3d::LightTiler::Init()
{
    // Light index buffer
    {
        const unsigned numTiles = GetNumTilesX() * GetNumTilesY();
        const unsigned maxNumLightsPerTile = GetMaxNumLightsPerTile();

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = maxNumLightsPerTile * numTiles * sizeof( unsigned );
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &perTileLightIndexBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)perTileLightIndexBuffer, VK_OBJECT_TYPE_BUFFER, "perTileLightIndexBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, perTileLightIndexBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &perTileLightIndexBufferMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory perTileLightIndexBuffer" );

        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)perTileLightIndexBufferMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "perTileLightIndexBufferMemory" );

        err = vkBindBufferMemory( GfxDeviceGlobal::device, perTileLightIndexBuffer, perTileLightIndexBufferMemory, 0 );
        AE3D_CHECK_VULKAN( err, "vkBindBufferMemory perTileLightIndexBuffer" );

        VkBufferViewCreateInfo bufferViewInfo = {};
        bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewInfo.flags = 0;
        bufferViewInfo.buffer = perTileLightIndexBuffer;
        bufferViewInfo.range = VK_WHOLE_SIZE;
        bufferViewInfo.format = VK_FORMAT_R32_UINT;

        err = vkCreateBufferView( GfxDeviceGlobal::device, &bufferViewInfo, nullptr, &perTileLightIndexBufferView );
        AE3D_CHECK_VULKAN( err, "light index buffer view" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)perTileLightIndexBufferView, VK_OBJECT_TYPE_BUFFER_VIEW, "perTileLightIndexBufferView" );
    }

    // Point light center/radius buffer
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MaxLights * 4 * sizeof( float );
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &pointLightCenterAndRadiusBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightCenterAndRadiusBuffer, VK_OBJECT_TYPE_BUFFER, "pointLightCenterAndRadiusBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, pointLightCenterAndRadiusBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &pointLightCenterAndRadiusMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory pointLightCenterAndRadiusMemory" );

        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightCenterAndRadiusMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "pointLightCenterAndRadiusMemory" );

        err = vkBindBufferMemory( GfxDeviceGlobal::device, pointLightCenterAndRadiusBuffer, pointLightCenterAndRadiusMemory, 0 );
        AE3D_CHECK_VULKAN( err, "vkBindBufferMemory pointLightCenterAndRadiusBuffer" );

        err = vkMapMemory( GfxDeviceGlobal::device, pointLightCenterAndRadiusMemory, 0, bufferInfo.size, 0, &mappedPointLightCenterAndRadiusMemory );
        AE3D_CHECK_VULKAN( err, "vkMapMemory point lights" );

        VkBufferViewCreateInfo bufferViewInfo = {};
        bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewInfo.flags = 0;
        bufferViewInfo.buffer = pointLightCenterAndRadiusBuffer;
        bufferViewInfo.range = VK_WHOLE_SIZE;
        bufferViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

        err = vkCreateBufferView( GfxDeviceGlobal::device, &bufferViewInfo, nullptr, &pointLightBufferView );
        AE3D_CHECK_VULKAN( err, "point light buffer view" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightBufferView, VK_OBJECT_TYPE_BUFFER_VIEW, "pointLightBufferView" );
    }

    // Point light color buffer
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MaxLights * 4 * sizeof( float );
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &pointLightColorBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightColorBuffer, VK_OBJECT_TYPE_BUFFER, "pointLightColorBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, pointLightColorBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &pointLightColorMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory pointLightColorMemory" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightColorMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "pointLightColorMemory" );

        err = vkBindBufferMemory( GfxDeviceGlobal::device, pointLightColorBuffer, pointLightColorMemory, 0 );
        AE3D_CHECK_VULKAN( err, "vkBindBufferMemory pointLightColorBuffer" );

        err = vkMapMemory( GfxDeviceGlobal::device, pointLightColorMemory, 0, bufferInfo.size, 0, &mappedPointLightColorMemory );
        AE3D_CHECK_VULKAN( err, "vkMapMemory point lights" );

        VkBufferViewCreateInfo bufferViewInfo = {};
        bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewInfo.flags = 0;
        bufferViewInfo.buffer = pointLightColorBuffer;
        bufferViewInfo.range = VK_WHOLE_SIZE;
        bufferViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

        err = vkCreateBufferView( GfxDeviceGlobal::device, &bufferViewInfo, nullptr, &pointLightColorView );
        AE3D_CHECK_VULKAN( err, "point light color view" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightColorView, VK_OBJECT_TYPE_BUFFER_VIEW, "pointLightColorView" );
    }

    // Spot light buffer
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MaxLights * 4 * sizeof( float );
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &spotLightCenterAndRadiusBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightCenterAndRadiusBuffer, VK_OBJECT_TYPE_BUFFER, "spotLightCenterAndRadiusBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, spotLightCenterAndRadiusBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &spotLightCenterAndRadiusMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory spotLightCenterAndRadiusMemory" );

        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightCenterAndRadiusMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "spotLightCenterAndRadiusMemory" );

        err = vkBindBufferMemory( GfxDeviceGlobal::device, spotLightCenterAndRadiusBuffer, spotLightCenterAndRadiusMemory, 0 );
        AE3D_CHECK_VULKAN( err, "vkBindBufferMemory spotLightCenterAndRadiusBuffer" );

        err = vkMapMemory( GfxDeviceGlobal::device, spotLightCenterAndRadiusMemory, 0, bufferInfo.size, 0, &mappedSpotLightCenterAndRadiusMemory );
        AE3D_CHECK_VULKAN( err, "vkMapMemory spot lights" );

        VkBufferViewCreateInfo bufferViewInfo = {};
        bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewInfo.flags = 0;
        bufferViewInfo.buffer = spotLightCenterAndRadiusBuffer;
        bufferViewInfo.range = VK_WHOLE_SIZE;
        bufferViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

        err = vkCreateBufferView( GfxDeviceGlobal::device, &bufferViewInfo, nullptr, &spotLightBufferView );
        AE3D_CHECK_VULKAN( err, "spot light buffer view" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightBufferView, VK_OBJECT_TYPE_BUFFER_VIEW, "spotLightBufferView" );
    }

    // Spot light params buffer
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MaxLights * 4 * sizeof( float );
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &spotLightParamsBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightParamsBuffer, VK_OBJECT_TYPE_BUFFER, "spotLightParamsBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, spotLightParamsBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &spotLightParamsMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory spotLightParamsMemory" );

        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightParamsMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "spotLightParamsMemory" );

        err = vkBindBufferMemory( GfxDeviceGlobal::device, spotLightParamsBuffer, spotLightParamsMemory, 0 );
        AE3D_CHECK_VULKAN( err, "vkBindBufferMemory spotLightParamsBuffer" );

        err = vkMapMemory( GfxDeviceGlobal::device, spotLightParamsMemory, 0, bufferInfo.size, 0, &mappedSpotLightParamsMemory );
        AE3D_CHECK_VULKAN( err, "vkMapMemory spot lights" );

        VkBufferViewCreateInfo bufferViewInfo = {};
        bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewInfo.flags = 0;
        bufferViewInfo.buffer = spotLightParamsBuffer;
        bufferViewInfo.range = VK_WHOLE_SIZE;
        bufferViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

        err = vkCreateBufferView( GfxDeviceGlobal::device, &bufferViewInfo, nullptr, &spotLightParamsView );
        AE3D_CHECK_VULKAN( err, "spot light params buffer view" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightParamsView, VK_OBJECT_TYPE_BUFFER_VIEW, "spotLightParamsBufferView" );
    }

    // Spot light color buffer
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MaxLights * 4 * sizeof( float );
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &spotLightColorBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightColorBuffer, VK_OBJECT_TYPE_BUFFER, "spotLightColorBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, spotLightColorBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &spotLightColorMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory spotLightColorMemory" );

        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightColorMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, "spotLightColorMemory" );

        err = vkBindBufferMemory( GfxDeviceGlobal::device, spotLightColorBuffer, spotLightColorMemory, 0 );
        AE3D_CHECK_VULKAN( err, "vkBindBufferMemory spotLightColorBuffer" );

        err = vkMapMemory( GfxDeviceGlobal::device, spotLightColorMemory, 0, bufferInfo.size, 0, &mappedSpotLightColorMemory );
        AE3D_CHECK_VULKAN( err, "vkMapMemory spot lights" );

        VkBufferViewCreateInfo bufferViewInfo = {};
        bufferViewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewInfo.flags = 0;
        bufferViewInfo.buffer = spotLightColorBuffer;
        bufferViewInfo.range = VK_WHOLE_SIZE;
        bufferViewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

        err = vkCreateBufferView( GfxDeviceGlobal::device, &bufferViewInfo, nullptr, &spotLightColorView );
        AE3D_CHECK_VULKAN( err, "spot light color view" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightColorView, VK_OBJECT_TYPE_BUFFER_VIEW, "spotLightColorView" );
    }
}

void ae3d::LightTiler::UpdateLightBuffers()
{
    std::memcpy( mappedPointLightCenterAndRadiusMemory, &pointLightCenterAndRadius[ 0 ], MaxLights * 4 * sizeof( float ) );
    std::memcpy( mappedPointLightColorMemory, &pointLightColors[ 0 ], MaxLights * 4 * sizeof( float ) );
    std::memcpy( mappedSpotLightCenterAndRadiusMemory, &spotLightCenterAndRadius[ 0 ], MaxLights * 4 * sizeof( float ) );
    std::memcpy( mappedSpotLightParamsMemory, &spotLightParams[ 0 ], MaxLights * 4 * sizeof( float ) );
    std::memcpy( mappedSpotLightColorMemory, &spotLightColors[ 0 ], MaxLights * 4 * sizeof( float ) );
}

unsigned ae3d::LightTiler::GetNumTilesX() const
{
    return (unsigned)((GfxDevice::backBufferWidth + TileRes - 1) / (float)TileRes);
}

unsigned ae3d::LightTiler::GetNumTilesY() const
{
    return (unsigned)((GfxDevice::backBufferHeight + TileRes - 1) / (float)TileRes);
}

void ae3d::LightTiler::CullLights( ComputeShader& shader, const Matrix44& projection, const Matrix44& localToView, RenderTexture& depthNormalTarget )
{
    Matrix44::Invert( projection, GfxDeviceGlobal::perObjectUboStruct.clipToView );

    GfxDeviceGlobal::perObjectUboStruct.localToView = localToView;
    GfxDeviceGlobal::perObjectUboStruct.windowWidth = depthNormalTarget.GetWidth();
    GfxDeviceGlobal::perObjectUboStruct.windowHeight = depthNormalTarget.GetHeight();
    GfxDeviceGlobal::perObjectUboStruct.numLights = (((unsigned)activeSpotLights & 0xFFFFu) << 16) | ((unsigned)activePointLights & 0xFFFFu);
    GfxDeviceGlobal::perObjectUboStruct.maxNumLightsPerTile = GetMaxNumLightsPerTile();
    GfxDeviceGlobal::perObjectUboStruct.tilesXY.x = (float)GetNumTilesX();
    GfxDeviceGlobal::perObjectUboStruct.tilesXY.y = (float)GetNumTilesY();
    
    GfxDeviceGlobal::boundViews[ 0 ] = depthNormalTarget.GetColorView();
    GfxDeviceGlobal::boundSamplers[ 0 ] = depthNormalTarget.GetSampler();
    
    shader.Begin();

    const unsigned numTiles = GetNumTilesX() * GetNumTilesY();
    const unsigned maxNumLightsPerTile = GetMaxNumLightsPerTile();

    VkBufferMemoryBarrier lightIndexToCompute = {};
    lightIndexToCompute.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    lightIndexToCompute.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    lightIndexToCompute.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    lightIndexToCompute.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    lightIndexToCompute.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    lightIndexToCompute.buffer = perTileLightIndexBuffer;
    lightIndexToCompute.size = maxNumLightsPerTile * numTiles * sizeof( unsigned );

    vkCmdPipelineBarrier( GfxDeviceGlobal::computeCmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                                     nullptr, 1, &lightIndexToCompute, 0, nullptr );
    
    shader.Dispatch( GetNumTilesX(), GetNumTilesY(), 1, "LightCuller" );

    VkBufferMemoryBarrier lightIndexToFrag = {};
    lightIndexToFrag.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    lightIndexToFrag.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    lightIndexToFrag.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    lightIndexToFrag.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    lightIndexToFrag.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    lightIndexToFrag.buffer = perTileLightIndexBuffer;
    lightIndexToFrag.size = maxNumLightsPerTile * numTiles * sizeof( unsigned );

    vkCmdPipelineBarrier( GfxDeviceGlobal::computeCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                     nullptr, 1, &lightIndexToFrag, 0, nullptr );
    
    shader.End();
}
