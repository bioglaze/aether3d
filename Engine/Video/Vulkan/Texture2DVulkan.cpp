#include "Texture2D.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "DDSLoader.hpp"
#include "FileSystem.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "VulkanUtils.hpp"

bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp
void Tokenize( const std::string& str,
std::vector< std::string >& tokens,
const std::string& delimiters = " " ); // Defined in TextureCommon.cpp

namespace ae3d
{
    void GetMemoryType( std::uint32_t typeBits, VkFlags properties, std::uint32_t* typeIndex ); // Defined in GfxDeviceVulkan.cpp 
}

namespace MathUtil
{
    int GetMipmapCount( int width, int height );
}

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkPhysicalDevice physicalDevice;
    extern VkQueue graphicsQueue;
    extern VkCommandPool cmdPool;
}

namespace Texture2DGlobal
{
    ae3d::Texture2D defaultTexture;
    VkCommandBuffer texCmdBuffer = VK_NULL_HANDLE;
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

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace, float aAnisotropy )
{
    // TODO: Move somewhere else.
    if (Texture2DGlobal::texCmdBuffer == VK_NULL_HANDLE)
    {
        System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );
        System::Assert( GfxDeviceGlobal::cmdPool != VK_NULL_HANDLE, "cmdPool not initialized" );

        VkCommandBufferAllocateInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufInfo.commandPool = GfxDeviceGlobal::cmdPool;
        cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufInfo.commandBufferCount = 1;

        VkResult err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &cmdBufInfo, &Texture2DGlobal::texCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers Texture2D" );
    }

    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    anisotropy = aAnisotropy;
    colorSpace = aColorSpace;
    handle = 1;
    width = 256;
    height = 256;
    path = fileContents.path;

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
    else if (isDDS)
    {
        LoadDDS( fileContents.path.c_str() );
    }
    else
    {
        ae3d::System::Print( "Unknown texture file extension: %s\n", fileContents.path.c_str() );
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

void ae3d::Texture2D::CreateVulkanObjects( void* data, int bytesPerPixel, VkFormat format )
{
    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.memoryTypeIndex = 0;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkDeviceSize bc1BlockSize = opaque ? 8 : 16;
    VkDeviceSize bc1Size = (width / 4) * (height / 4) * bc1BlockSize;
    VkDeviceSize imageSize = (isBC1( format ) || isBC2( format ) || isBC3( format )) ? bc1Size : (width * height * bytesPerPixel);

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = imageSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferCreateInfo, nullptr, &stagingBuffer );
    AE3D_CHECK_VULKAN( err, "vkCreateBuffer staging" );

    VkMemoryRequirements memReqs = {};
    vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, stagingBuffer, &memReqs );

    memAllocInfo.allocationSize = memReqs.size;
    GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex );

    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &stagingMemory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory staging" );

    err = vkBindBufferMemory( GfxDeviceGlobal::device, stagingBuffer, stagingMemory, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindBufferMemory staging" );

    std::uint8_t* stagingData;
    err = vkMapMemory( GfxDeviceGlobal::device, stagingMemory, 0, memReqs.size, 0, (void **)&stagingData );
    std::memcpy( stagingData, data, imageSize );
    vkUnmapMemory( GfxDeviceGlobal::device, stagingMemory );

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    std::uint32_t offset = 0;

    mipLevelCount = mipmaps == Mipmaps::Generate ? static_cast<int>(MathUtil::GetMipmapCount( width, height )) : 1;

    // We're generating mips at runtime, so no need to loop.
    for (int i = 0; i < 1/*mipLevels*/; ++i)
    {
        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = i;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = width;
        bufferCopyRegion.imageExtent.height = height;
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;

        bufferCopyRegions.push_back( bufferCopyRegion );

        offset += width * height * bytesPerPixel;
    }

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
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageCreateInfo.extent = { static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &image );
    AE3D_CHECK_VULKAN( err, "vkCreateImage" );
    Texture2DGlobal::imagesToReleaseAtExit.push_back( image );

    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, image, &memReqs );

    memAllocInfo.allocationSize = memReqs.size;
    GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex );

    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory" );
    Texture2DGlobal::memoryToReleaseAtExit.push_back( deviceMemory );

    err = vkBindImageMemory( GfxDeviceGlobal::device, image, deviceMemory, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindImageMemory" );

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;
    cmdBufInfo.pInheritanceInfo = nullptr;
    cmdBufInfo.flags = 0;

    err = vkBeginCommandBuffer( Texture2DGlobal::texCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer in Texture2D" );

    SetImageLayout(
        Texture2DGlobal::texCmdBuffer,
        image,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        0,
        mipLevelCount );

    vkCmdCopyBufferToImage(
        Texture2DGlobal::texCmdBuffer,
        stagingBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<std::uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data()
    );

    for (int i = 1; i < mipLevelCount; ++i)
    {
        const std::int32_t mipWidth = width >> i;
        const std::int32_t mipHeight = height >> i;

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

        vkCmdBlitImage( Texture2DGlobal::texCmdBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR );
    }

    auto imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    SetImageLayout(
        Texture2DGlobal::texCmdBuffer,
        image,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        imageLayout,
        1,
        0,
        mipLevelCount );

    err = vkEndCommandBuffer( Texture2DGlobal::texCmdBuffer );
    AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer in Texture2D" );

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &Texture2DGlobal::texCmdBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit in Texture2D" );

    err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
    AE3D_CHECK_VULKAN( err, "vkQueueWaitIdle in Texture2D" );

    vkFreeMemory( GfxDeviceGlobal::device, stagingMemory, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, stagingBuffer, nullptr );

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.levelCount = mipLevelCount;
    viewInfo.image = image;
    err = vkCreateImageView( GfxDeviceGlobal::device, &viewInfo, nullptr, &view );
    AE3D_CHECK_VULKAN( err, "vkCreateImageView in Texture2D" );
    Texture2DGlobal::imageViewsToReleaseAtExit.push_back( view );

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext = nullptr;
    samplerInfo.magFilter = filter == ae3d::TextureFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerInfo.minFilter = samplerInfo.magFilter;
    samplerInfo.mipmapMode = filter == ae3d::TextureFilter::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU =  wrap == ae3d::TextureWrap::Repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.mipLodBias = 0;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = static_cast< float >(mipLevelCount);
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    err = vkCreateSampler( GfxDeviceGlobal::device, &samplerInfo, nullptr, &sampler );
    AE3D_CHECK_VULKAN( err, "vkCreateSampler" );
    Texture2DGlobal::samplersToReleaseAtExit.push_back( sampler );

    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "sampler" );
}

void ae3d::Texture2D::LoadDDS( const char* aPath )
{
    DDSLoader::Output ddsOutput;
    auto fileContents = FileSystem::FileContents( aPath );
    const DDSLoader::LoadResult loadResult = DDSLoader::Load( fileContents, 0, width, height, opaque, ddsOutput );

    if (loadResult != DDSLoader::LoadResult::Success)
    {
        ae3d::System::Print( "DDS Loader could not load %s", aPath );
        return;
    }

    mipLevelCount = static_cast< int >(ddsOutput.dataOffsets.size());
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
    
    ae3d::System::Assert( ddsOutput.dataOffsets.size() > 0, "DDS reader error: dataoffsets is empty" );

    CreateVulkanObjects( &fileContents.data[ ddsOutput.dataOffsets[ 0 ] ], bytesPerPixel, format );
}

void ae3d::Texture2D::LoadSTB( const FileSystem::FileContentsData& fileContents )
{
    System::Assert( GfxDeviceGlobal::graphicsQueue != VK_NULL_HANDLE, "queue not initialized" );
    System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );
    System::Assert( Texture2DGlobal::texCmdBuffer != VK_NULL_HANDLE, "texCmdBuffer not initialized" );

    int components;
    unsigned char* data = stbi_load_from_memory( fileContents.data.data(), static_cast<int>(fileContents.data.size()), &width, &height, &components, 4 );

    if (data == nullptr)
    {
        const std::string reason( stbi_failure_reason() );
        System::Print( "%s failed to load. stb_image's reason: %s\n", fileContents.path.c_str(), reason.c_str() );
        return;
    }

    opaque = (components == 3 || components == 1);

    CreateVulkanObjects( data, 4, colorSpace == ColorSpace::RGB ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8A8_SRGB );

    stbi_image_free( data );
}

ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    Texture2DGlobal::defaultTexture.handle = 1;
    Texture2DGlobal::defaultTexture.width = 256;
    Texture2DGlobal::defaultTexture.height = 256;
    return &Texture2DGlobal::defaultTexture;
}
