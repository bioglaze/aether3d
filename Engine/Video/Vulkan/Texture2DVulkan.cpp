#include "Texture2D.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "Array.hpp"
#include "DDSLoader.hpp"
#include "FileSystem.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "Statistics.hpp"
#include "VulkanUtils.hpp"

bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp
float GetFloatAnisotropy( ae3d::Anisotropy anisotropy );

namespace MathUtil
{
    int GetMipmapCount( int width, int height );
    int Max( int a, int b );
    bool IsPowerOfTwo( unsigned i );
}

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkPhysicalDevice physicalDevice;
    extern VkQueue graphicsQueue;
    extern VkCommandPool cmdPool;
    extern VkPhysicalDeviceProperties properties;
    extern VkCommandBuffer texCmdBuffer;
    extern VkPhysicalDeviceFeatures deviceFeatures;
}

namespace Texture2DGlobal
{
    ae3d::Texture2D defaultTexture;
    ae3d::Texture2D defaultTextureUAV;
    std::vector< VkSampler > samplersToReleaseAtExit;
    std::vector< VkImage > imagesToReleaseAtExit;
    std::vector< VkImageView > imageViewsToReleaseAtExit;
    std::vector< VkDeviceMemory > memoryToReleaseAtExit;
}

void ae3d::Texture2D::DestroyTextures()
{
    for (std::size_t samplerIndex = 0; samplerIndex < Texture2DGlobal::samplersToReleaseAtExit.size(); ++samplerIndex)
    {
        vkDestroySampler( GfxDeviceGlobal::device, Texture2DGlobal::samplersToReleaseAtExit[ samplerIndex ], nullptr );
    }

    for (std::size_t imageIndex = 0; imageIndex < Texture2DGlobal::imagesToReleaseAtExit.size(); ++imageIndex)
    {
        vkDestroyImage( GfxDeviceGlobal::device, Texture2DGlobal::imagesToReleaseAtExit[ imageIndex ], nullptr );
    }

    for (std::size_t imageViewIndex = 0; imageViewIndex < Texture2DGlobal::imageViewsToReleaseAtExit.size(); ++imageViewIndex)
    {
        vkDestroyImageView( GfxDeviceGlobal::device, Texture2DGlobal::imageViewsToReleaseAtExit[ imageViewIndex ], nullptr );
    }

    for (std::size_t memoryIndex = 0; memoryIndex < Texture2DGlobal::memoryToReleaseAtExit.size(); ++memoryIndex)
    {
        vkFreeMemory( GfxDeviceGlobal::device, Texture2DGlobal::memoryToReleaseAtExit[ memoryIndex ], nullptr );
    }
}

void ae3d::Texture2D::LoadFromData( const void* imageData, int aWidth, int aHeight, int channels, const char* debugName )
{
    width = aWidth;
    height = aHeight;
    wrap = TextureWrap::Repeat;
    filter = TextureFilter::Linear;
    opaque = channels == 3;

    CreateVulkanObjects( const_cast< void* >( imageData ), 4, colorSpace == ColorSpace::RGB ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_SRGB );

    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)view, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, debugName );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, debugName );
}

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace, Anisotropy aAnisotropy )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    anisotropy = aAnisotropy;
    colorSpace = aColorSpace;
    handle = 1;
    width = 256;
    height = 256;
    path = fileContents.path;

    if (static_cast<int>(GfxDeviceGlobal::properties.limits.maxSamplerAnisotropy) < GetFloatAnisotropy( anisotropy ))
    {
        System::Print( "%s is using too big anisotropy (%f), max supported is %fx.\n", fileContents.path.c_str(), (double)GetFloatAnisotropy( anisotropy ),
                       (double)GfxDeviceGlobal::properties.limits.maxSamplerAnisotropy );
        anisotropy = Anisotropy::k4;
    }

    if (!fileContents.isLoaded)
    {
        *this = Texture2DGlobal::defaultTexture;
        return;
    }

    const bool isDDS = fileContents.path.find( ".dds" ) != std::string::npos || fileContents.path.find( ".DDS" ) != std::string::npos;

    if (HasStbExtension( fileContents.path ))
    {
        LoadSTB( fileContents );
    }
    else if (isDDS && GfxDeviceGlobal::deviceFeatures.textureCompressionBC)
    {
        LoadDDS( fileContents.path.c_str() );
    }
    else
    {
        System::Print( "Unknown/unsupported texture file extension: %s\n", fileContents.path.c_str() );
    }

    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)view, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, fileContents.path.c_str() );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, fileContents.path.c_str() );
}

bool isBC1( VkFormat format )
{
    return format == VK_FORMAT_BC1_RGB_UNORM_BLOCK || format == VK_FORMAT_BC1_RGB_SRGB_BLOCK ||
           format == VK_FORMAT_BC1_RGBA_UNORM_BLOCK || format == VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
}

bool isBC2( VkFormat format )
{
    return format == VK_FORMAT_BC2_UNORM_BLOCK || format == VK_FORMAT_BC2_SRGB_BLOCK;
}

bool isBC3( VkFormat format )
{
    return format == VK_FORMAT_BC3_UNORM_BLOCK || format == VK_FORMAT_BC3_SRGB_BLOCK;
}

void ae3d::Texture2D::CreateVulkanObjects( const DDSLoader::Output& mipChain, int bytesPerPixel, VkFormat format )
{
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = mipLevelCount;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { (std::uint32_t)width, (std::uint32_t)height, 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkResult err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &image );
    AE3D_CHECK_VULKAN( err, "vkCreateImage" );
    Texture2DGlobal::imagesToReleaseAtExit.push_back( image );

    VkMemoryRequirements memReqs = {};
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, image, &memReqs );

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory" );
    Statistics::IncAllocCalls();
    Statistics::IncTotalAllocCalls();
    Texture2DGlobal::memoryToReleaseAtExit.push_back( deviceMemory );

    err = vkBindImageMemory( GfxDeviceGlobal::device, image, deviceMemory, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindImageMemory" );

    Array< VkBuffer > stagingBuffers;
    stagingBuffers.Allocate( mipLevelCount );
    Array< VkDeviceMemory > stagingMemory;
    stagingMemory.Allocate( mipLevelCount );
    
    for (int mipIndex = 0; mipIndex < mipLevelCount; ++mipIndex)
    {
        const std::int32_t mipWidth = MathUtil::Max( width >> mipIndex, 1 );
        const std::int32_t mipHeight = MathUtil::Max( height >> mipIndex, 1 );

        const VkDeviceSize bc1BlockSize = opaque ? 8 : 16;
        const VkDeviceSize bc1Size = (mipWidth / 4) * (mipHeight / 4) * bc1BlockSize;
        VkDeviceSize imageSize = (isBC1( format ) || isBC2( format ) || isBC3( format )) ? bc1Size : (mipWidth * mipHeight * bytesPerPixel);

        // FIXME: This is a hack, figure out proper fix.
        if (imageSize == 0)
        {
            imageSize = 16;
        }
        
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = imageSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferCreateInfo, nullptr, &stagingBuffers[ mipIndex ] );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer staging" );

        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, stagingBuffers[ mipIndex ], &memReqs );

        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &stagingMemory[ mipIndex ] );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory" );

        err = vkBindBufferMemory( GfxDeviceGlobal::device, stagingBuffers[ mipIndex ], stagingMemory[ mipIndex ], 0 );
        AE3D_CHECK_VULKAN( err, "vkBindBufferMemory staging" );

        void* stagingData;
        err = vkMapMemory( GfxDeviceGlobal::device, stagingMemory[ mipIndex ], 0, memReqs.size, 0, &stagingData );
        AE3D_CHECK_VULKAN( err, "vkMapMemory in Texture2D" );
        VkDeviceSize amountToCopy = imageSize;
        if (mipChain.dataOffsets[ mipIndex ] + imageSize >= (unsigned)mipChain.imageData.count)
        {
            amountToCopy = mipChain.imageData.count - mipChain.dataOffsets[ mipIndex ];
        }
        
        std::memcpy( stagingData, &mipChain.imageData[ mipChain.dataOffsets[ mipIndex ] ], amountToCopy );

        VkMappedMemoryRange flushRange = {};
        flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flushRange.pNext = nullptr;
        flushRange.memory = stagingMemory[ mipIndex ];
        flushRange.offset = 0;
        flushRange.size = VK_WHOLE_SIZE;
        vkFlushMappedMemoryRanges( GfxDeviceGlobal::device, 1, &flushRange );

        vkUnmapMemory( GfxDeviceGlobal::device, stagingMemory[ mipIndex ] );
    }

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.levelCount = mipLevelCount;
    viewInfo.image = image;
    err = vkCreateImageView( GfxDeviceGlobal::device, &viewInfo, nullptr, &view );
    AE3D_CHECK_VULKAN( err, "vkCreateImageView in Texture2D" );
    Texture2DGlobal::imageViewsToReleaseAtExit.push_back( view );

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;
    cmdBufInfo.pInheritanceInfo = nullptr;
    cmdBufInfo.flags = 0;

    err = vkBeginCommandBuffer( GfxDeviceGlobal::texCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer in Texture2D" );

    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevelCount;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = range;

    vkCmdPipelineBarrier(
            GfxDeviceGlobal::texCmdBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier );

    for (int mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel)
    {
        const std::int32_t mipWidth = MathUtil::Max( width >> mipLevel, 1 );
        const std::int32_t mipHeight = MathUtil::Max( height >> mipLevel, 1 );

        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = mipLevel;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = mipWidth;
        bufferCopyRegion.imageExtent.height = mipHeight;
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = 0;

        vkCmdCopyBufferToImage( GfxDeviceGlobal::texCmdBuffer, stagingBuffers[ mipLevel ], image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion );
    }

    imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = range;
    
    vkCmdPipelineBarrier(
        GfxDeviceGlobal::texCmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier );

    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = range;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = mipLevelCount;

    vkCmdPipelineBarrier(
            GfxDeviceGlobal::texCmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier );

    vkEndCommandBuffer( GfxDeviceGlobal::texCmdBuffer );

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::texCmdBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit in Texture2D" );

    vkDeviceWaitIdle( GfxDeviceGlobal::device );

    for (int mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel)
    {
        vkDestroyBuffer( GfxDeviceGlobal::device, stagingBuffers[ mipLevel ], nullptr );
        vkFreeMemory( GfxDeviceGlobal::device, stagingMemory[ mipLevel ], nullptr );
    }
    
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = nullptr;
    samplerInfo.magFilter = filter == ae3d::TextureFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerInfo.minFilter = samplerInfo.magFilter;
    samplerInfo.mipmapMode = filter == ae3d::TextureFilter::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = wrap == ae3d::TextureWrap::Repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.mipLodBias = 0;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = static_cast< float >(mipLevelCount);
    samplerInfo.maxAnisotropy = GfxDeviceGlobal::deviceFeatures.samplerAnisotropy ? GetFloatAnisotropy( anisotropy ) : 1;
    samplerInfo.anisotropyEnable = (anisotropy != Anisotropy::k1 && GfxDeviceGlobal::deviceFeatures.samplerAnisotropy) ? VK_TRUE : VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    err = vkCreateSampler( GfxDeviceGlobal::device, &samplerInfo, nullptr, &sampler );
    AE3D_CHECK_VULKAN( err, "vkCreateSampler" );
    Texture2DGlobal::samplersToReleaseAtExit.push_back( sampler );

    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "sampler" );
}

void ae3d::Texture2D::CreateVulkanObjects( void* data, int bytesPerPixel, VkFormat format )
{
    if (!MathUtil::IsPowerOfTwo( width ) || !MathUtil::IsPowerOfTwo( height ))
    {
        if (mipmaps == Mipmaps::Generate)
        {
            System::Print( "Mipmaps not generated for %s because the dimension (%dx%d) is not power-of-two.\n", path.c_str(), width, height );
        }

        mipmaps = Mipmaps::None;
    }

    mipLevelCount = mipmaps == Mipmaps::Generate ? MathUtil::GetMipmapCount( width, height ) : 1;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = mipLevelCount;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.extent = { (std::uint32_t)width, (std::uint32_t)height, 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkResult err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &image );
    AE3D_CHECK_VULKAN( err, "vkCreateImage" );
    Texture2DGlobal::imagesToReleaseAtExit.push_back( image );

    VkMemoryRequirements memReqs = {};
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, image, &memReqs );

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory" );
    Statistics::IncAllocCalls();
    Statistics::IncTotalAllocCalls();
    Texture2DGlobal::memoryToReleaseAtExit.push_back( deviceMemory );

    err = vkBindImageMemory( GfxDeviceGlobal::device, image, deviceMemory, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindImageMemory" );

    VkBuffer stagingBuffer = VK_NULL_HANDLE;

    VkDeviceSize imageSize = width * height * bytesPerPixel;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = imageSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferCreateInfo, nullptr, &stagingBuffer );
    AE3D_CHECK_VULKAN( err, "vkCreateBuffer staging" );

    vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, stagingBuffer, &memReqs );

    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &stagingMemory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory" );

    err = vkBindBufferMemory( GfxDeviceGlobal::device, stagingBuffer, stagingMemory, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindBufferMemory staging" );

    void* stagingData;
    err = vkMapMemory( GfxDeviceGlobal::device, stagingMemory, 0, memReqs.size, 0, &stagingData );
    AE3D_CHECK_VULKAN( err, "vkMapMemory in Texture2D" );
    std::memcpy( stagingData, data, imageSize );

    VkMappedMemoryRange flushRange = {};
    flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    flushRange.pNext = nullptr;
    flushRange.memory = stagingMemory;
    flushRange.offset = 0;
    flushRange.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges( GfxDeviceGlobal::device, 1, &flushRange );

    vkUnmapMemory( GfxDeviceGlobal::device, stagingMemory );

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.levelCount = mipLevelCount;
    viewInfo.image = image;
    err = vkCreateImageView( GfxDeviceGlobal::device, &viewInfo, nullptr, &view );
    AE3D_CHECK_VULKAN( err, "vkCreateImageView in Texture2D" );
    Texture2DGlobal::imageViewsToReleaseAtExit.push_back( view );

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;
    cmdBufInfo.pInheritanceInfo = nullptr;
    cmdBufInfo.flags = 0;

    err = vkBeginCommandBuffer( GfxDeviceGlobal::texCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer in Texture2D" );

    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevelCount;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = range;

    vkCmdPipelineBarrier(
            GfxDeviceGlobal::texCmdBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier );

    VkBufferImageCopy bufferCopyRegion = {};
    bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferCopyRegion.imageSubresource.mipLevel = 0;
    bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
    bufferCopyRegion.imageSubresource.layerCount = 1;
    bufferCopyRegion.imageExtent.width = width;
    bufferCopyRegion.imageExtent.height = height;
    bufferCopyRegion.imageExtent.depth = 1;
    bufferCopyRegion.bufferOffset = 0;

    vkCmdCopyBufferToImage( GfxDeviceGlobal::texCmdBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion );

    imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = range;

    vkCmdPipelineBarrier(
        GfxDeviceGlobal::texCmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier );

    vkEndCommandBuffer( GfxDeviceGlobal::texCmdBuffer );

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::texCmdBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit in Texture2D" );

    vkDeviceWaitIdle( GfxDeviceGlobal::device );

    err = vkBeginCommandBuffer( GfxDeviceGlobal::texCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer in Texture2D" );

    for (int i = 1; i < mipLevelCount; ++i)
    {
        const std::int32_t mipWidth = MathUtil::Max( width >> i, 1 );
        const std::int32_t mipHeight = MathUtil::Max( height >> i, 1 );

        VkImageBlit imageBlit = {};
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.baseArrayLayer = 0;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel = 0;
        imageBlit.srcOffsets[ 0 ] = { 0, 0, 0 };
        imageBlit.srcOffsets[ 1 ] = { width, height, 1 };

        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.baseArrayLayer = 0;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[ 0 ] = { 0, 0, 0 };
        imageBlit.dstOffsets[ 1 ] = { mipWidth, mipHeight, 1 };

        vkCmdBlitImage( GfxDeviceGlobal::texCmdBuffer, image, VK_IMAGE_LAYOUT_GENERAL, image,
            VK_IMAGE_LAYOUT_GENERAL, 1, &imageBlit, VK_FILTER_LINEAR );
    }

    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = range;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.levelCount = mipLevelCount;

    vkCmdPipelineBarrier(
            GfxDeviceGlobal::texCmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier );

    vkEndCommandBuffer( GfxDeviceGlobal::texCmdBuffer );

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit in Texture2D" );

    vkDeviceWaitIdle( GfxDeviceGlobal::device );
    vkFreeMemory( GfxDeviceGlobal::device, stagingMemory, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, stagingBuffer, nullptr );

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = nullptr;
    samplerInfo.magFilter = filter == ae3d::TextureFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerInfo.minFilter = samplerInfo.magFilter;
    samplerInfo.mipmapMode = filter == ae3d::TextureFilter::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = wrap == ae3d::TextureWrap::Repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.mipLodBias = 0;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = static_cast< float >(mipLevelCount);
    samplerInfo.maxAnisotropy = GfxDeviceGlobal::deviceFeatures.samplerAnisotropy ? GetFloatAnisotropy( anisotropy ) : 1;
    samplerInfo.anisotropyEnable = (anisotropy != Anisotropy::k1 && GfxDeviceGlobal::deviceFeatures.samplerAnisotropy) ? VK_TRUE : VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    err = vkCreateSampler( GfxDeviceGlobal::device, &samplerInfo, nullptr, &sampler );
    AE3D_CHECK_VULKAN( err, "vkCreateSampler" );
    Texture2DGlobal::samplersToReleaseAtExit.push_back( sampler );

    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "sampler" );
}

void ae3d::Texture2D::LoadDDS( const char* aPath )
{
    DDSLoader::Output ddsOutput;
    auto fileContents = FileSystem::FileContents( aPath );
    const DDSLoader::LoadResult loadResult = DDSLoader::Load( fileContents, width, height, opaque, ddsOutput );

    if (loadResult != DDSLoader::LoadResult::Success)
    {
        ae3d::System::Print( "DDS Loader could not load %s", aPath );
        return;
    }

    if (static_cast< int >( GfxDeviceGlobal::properties.limits.maxImageDimension2D ) < width ||
        static_cast< int >( GfxDeviceGlobal::properties.limits.maxImageDimension2D ) < height)
    {
        System::Print( "%s is too big (%dx%d), max supported size is %dx%d.\n", fileContents.path.c_str(), width, height,
            GfxDeviceGlobal::properties.limits.maxImageDimension2D, GfxDeviceGlobal::properties.limits.maxImageDimension2D );
        width = GfxDeviceGlobal::properties.limits.maxImageDimension2D;
        height = GfxDeviceGlobal::properties.limits.maxImageDimension2D;
    }

    mipLevelCount = mipmaps == Mipmaps::Generate ? ddsOutput.dataOffsets.count : 1;
    int bytesPerPixel = 1;

    VkFormat format = (colorSpace == ColorSpace::RGB) ? VK_FORMAT_BC1_RGB_UNORM_BLOCK : VK_FORMAT_BC1_RGB_SRGB_BLOCK;

    if (!opaque)
    {
        format = (colorSpace == ColorSpace::RGB) ? VK_FORMAT_BC1_RGBA_UNORM_BLOCK : VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
    }

    if (ddsOutput.format == DDSLoader::Format::BC2)
    {
        format = (colorSpace == ColorSpace::RGB) ? VK_FORMAT_BC2_UNORM_BLOCK : VK_FORMAT_BC2_SRGB_BLOCK;
        bytesPerPixel = 2;
    }
    else if (ddsOutput.format == DDSLoader::Format::BC3)
    {
        format = (colorSpace == ColorSpace::RGB) ? VK_FORMAT_BC3_UNORM_BLOCK : VK_FORMAT_BC3_SRGB_BLOCK;
        bytesPerPixel = 2;
    }
    
    ae3d::System::Assert( ddsOutput.dataOffsets.count > 0, "DDS reader error: dataoffsets is empty" );

    CreateVulkanObjects( ddsOutput, bytesPerPixel, format );
}

void ae3d::Texture2D::LoadSTB( const FileSystem::FileContentsData& fileContents )
{
    System::Assert( GfxDeviceGlobal::graphicsQueue != VK_NULL_HANDLE, "queue not initialized" );
    System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );

    int components;
    unsigned char* data = stbi_load_from_memory( fileContents.data.data(), static_cast<int>(fileContents.data.size()), &width, &height, &components, 4 );

    if (data == nullptr)
    {
        const std::string reason( stbi_failure_reason() );
        System::Print( "%s failed to load. stb_image's reason: %s\n", fileContents.path.c_str(), reason.c_str() );
        return;
    }

    if (static_cast< int >( GfxDeviceGlobal::properties.limits.maxImageDimension2D ) < width ||
        static_cast< int >( GfxDeviceGlobal::properties.limits.maxImageDimension2D ) < height)
    {
        System::Print( "%s is too big (%dx%d), max supported size is %dx%d.\n", fileContents.path.c_str(), width, height, 
            GfxDeviceGlobal::properties.limits.maxImageDimension2D, GfxDeviceGlobal::properties.limits.maxImageDimension2D );
        width = GfxDeviceGlobal::properties.limits.maxImageDimension2D;
        height = GfxDeviceGlobal::properties.limits.maxImageDimension2D;
    }

    opaque = (components == 3 || components == 1);

    CreateVulkanObjects( data, 4, colorSpace == ColorSpace::RGB ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_SRGB );

    stbi_image_free( data );
}

ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    if (Texture2DGlobal::defaultTexture.view == VK_NULL_HANDLE)
    {
        std::uint8_t imageData[ 32 * 32 * 4 ];
        
        for (int i = 0; i < 32 * 32 * 4; ++i)
        {
            imageData[ i ] = 0xFF;
        }

        Texture2DGlobal::defaultTexture.LoadFromData( imageData, 32, 32, 4, "default texture 2d" );
    }

    return &Texture2DGlobal::defaultTexture;
}

ae3d::Texture2D* ae3d::Texture2D::GetDefaultTextureUAV()
{
    if (Texture2DGlobal::defaultTextureUAV.view == VK_NULL_HANDLE)
    {
        std::uint8_t imageData[ 32 * 32 * 4 ];
        
        for (int i = 0; i < 32 * 32 * 4; ++i)
        {
            imageData[ i ] = 0xFF;
        }

        // TODO: Actually make this an UAV
        Texture2DGlobal::defaultTextureUAV.LoadFromData( imageData, 32, 32, 4, "default texture 2d UAV" );
    }

    return &Texture2DGlobal::defaultTextureUAV;
}
