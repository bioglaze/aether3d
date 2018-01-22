#include "LightTiler.hpp"
#include <cstring>
#include "ComputeShader.hpp"
#include "Macros.hpp"
#include "RenderTexture.hpp"
#include "Renderer.hpp"
#include "System.hpp"
#include "GfxDevice.hpp"
#include "VulkanUtils.hpp"
// TODO: Some of these methods can be shared with D3D12 and possibly Metal.

extern ae3d::Renderer renderer;

namespace MathUtil
{
    int Max( int x, int y );
}

namespace GfxDeviceGlobal
{
    extern int backBufferWidth;
    extern int backBufferHeight;
    extern VkDevice device;
    extern PerObjectUboStruct perObjectUboStruct;
    extern VkCommandBuffer computeCmdBuffer;
    extern VkPipelineCache pipelineCache;
    extern VkDescriptorSetLayout descriptorSetLayout;
    extern VkPipelineLayout pipelineLayout;
    extern VkQueue computeQueue;
    extern VkImageView view0;
    extern VkSampler sampler0;
}

void BindComputeDescriptorSet();
void UploadPerObjectUbo();

void ae3d::LightTiler::DestroyBuffers()
{
    vkDestroyBuffer( GfxDeviceGlobal::device, perTileLightIndexBuffer, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, pointLightCenterAndRadiusBuffer, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, pointLightColorBuffer, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, spotLightCenterAndRadiusBuffer, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, perTileLightIndexBufferView, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, pointLightBufferView, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, pointLightColorView, nullptr );
    vkDestroyBufferView( GfxDeviceGlobal::device, spotLightBufferView, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, perTileLightIndexBufferMemory, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, pointLightCenterAndRadiusMemory, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, pointLightColorMemory, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, spotLightCenterAndRadiusMemory, nullptr );
    vkDestroyPipeline( GfxDeviceGlobal::device, pso, nullptr );
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
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)perTileLightIndexBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "perTileLightIndexBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, perTileLightIndexBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &perTileLightIndexBufferMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory perTileLightIndexBuffer" );

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
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)perTileLightIndexBufferView, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, "perTileLightIndexBufferView" );
    }

    // Point light center/radius buffer
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MaxLights * 4 * sizeof( float );
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &pointLightCenterAndRadiusBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightCenterAndRadiusBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "pointLightCenterAndRadiusBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, pointLightCenterAndRadiusBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &pointLightCenterAndRadiusMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory pointLightCenterAndRadiusMemory" );

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
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightBufferView, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, "pointLightBufferView" );
    }

    // Point light color buffer
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MaxLights * 4 * sizeof( float );
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &pointLightColorBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightColorBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "pointLightColorBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, pointLightColorBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &pointLightColorMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory pointLightColorMemory" );

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
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pointLightColorView, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, "pointLightColorView" );
    }

    // Spot light buffer
    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MaxLights * 4 * sizeof( float );
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &spotLightCenterAndRadiusBuffer );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightCenterAndRadiusBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "spotLightCenterAndRadiusBuffer" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, spotLightCenterAndRadiusBuffer, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &spotLightCenterAndRadiusMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory spotLightCenterAndRadiusMemory" );

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
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)spotLightBufferView, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT, "spotLightBufferView" );
    }

    VkComputePipelineCreateInfo psoInfo = {};
    psoInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    psoInfo.layout = GfxDeviceGlobal::pipelineLayout;
    psoInfo.stage = renderer.builtinShaders.lightCullShader.GetInfo();
    
    VkResult err = vkCreateComputePipelines( GfxDeviceGlobal::device, GfxDeviceGlobal::pipelineCache, 1, &psoInfo, nullptr, &pso );
    AE3D_CHECK_VULKAN( err, "Light tiler PSO" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pso, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "light tiler PSO" );
}

void ae3d::LightTiler::UpdateLightBuffers()
{
    std::memcpy( mappedPointLightCenterAndRadiusMemory, &pointLightCenterAndRadius[ 0 ], MaxLights * 4 * sizeof( float ) );
    std::memcpy( mappedPointLightColorMemory, &pointLightColors[ 0 ], MaxLights * 4 * sizeof( float ) );
    std::memcpy( mappedSpotLightCenterAndRadiusMemory, &spotLightCenterAndRadius[ 0 ], MaxLights * 4 * sizeof( float ) );
}

unsigned ae3d::LightTiler::GetNumTilesX() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferWidth + TileRes - 1) / (float)TileRes);
}

unsigned ae3d::LightTiler::GetNumTilesY() const
{
    return (unsigned)((GfxDeviceGlobal::backBufferHeight + TileRes - 1) / (float)TileRes);
}

void ae3d::LightTiler::CullLights( ComputeShader& shader, const Matrix44& projection, const Matrix44& localToView, RenderTexture& depthNormalTarget )
{
    Matrix44::Invert( projection, GfxDeviceGlobal::perObjectUboStruct.clipToView );

    GfxDeviceGlobal::perObjectUboStruct.localToView = localToView;
    GfxDeviceGlobal::perObjectUboStruct.windowWidth = depthNormalTarget.GetWidth();
    GfxDeviceGlobal::perObjectUboStruct.windowHeight = depthNormalTarget.GetHeight();
    GfxDeviceGlobal::perObjectUboStruct.numLights = (((unsigned)activeSpotLights & 0xFFFFu) << 16) | ((unsigned)activePointLights & 0xFFFFu);
    GfxDeviceGlobal::perObjectUboStruct.maxNumLightsPerTile = GetMaxNumLightsPerTile();

    GfxDeviceGlobal::view0 = depthNormalTarget.GetColorView();
    GfxDeviceGlobal::sampler0 = depthNormalTarget.GetSampler();
    
    UploadPerObjectUbo();

    cullerUniformsCreated = true;

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::computeCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

    BindComputeDescriptorSet();

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
    
    vkCmdBindPipeline( GfxDeviceGlobal::computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pso );

    shader.Dispatch( GetNumTilesX(), GetNumTilesY(), 1 );

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
    
    vkEndCommandBuffer( GfxDeviceGlobal::computeCmdBuffer );

    VkPipelineStageFlags pipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &pipelineStages;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;//&GfxDeviceGlobal::presentCompleteSemaphore;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;//&GfxDeviceGlobal::renderCompleteSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::computeCmdBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::computeQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit compute" );

    err = vkQueueWaitIdle( GfxDeviceGlobal::computeQueue );
    AE3D_CHECK_VULKAN( err, "vkQueueWaitIdle" );
}
