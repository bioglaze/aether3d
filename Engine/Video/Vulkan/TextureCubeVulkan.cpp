#include "TextureCube.hpp"
#include <cstring>
#include <vulkan/vulkan.h>
#include "stb_image.c"
#include "DDSLoader.hpp"
#include "FileSystem.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "Statistics.hpp"
#include "VulkanUtils.hpp"

bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp

namespace MathUtil
{
    bool IsPowerOfTwo( unsigned i );
    int GetMipmapCount( int width, int height );
    int Max( int a, int b );
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

namespace TextureCubeGlobal
{
    ae3d::TextureCube defaultTexture;
    std::vector< VkSampler > samplersToReleaseAtExit;
    std::vector< VkImage > imagesToReleaseAtExit;
    std::vector< VkImageView > imageViewsToReleaseAtExit;
    std::vector< VkDeviceMemory > memoryToReleaseAtExit;
    std::vector< VkBuffer > buffersToReleaseAtExit;
}

void ae3d::TextureCube::DestroyTextures()
{
    for (std::size_t samplerIndex = 0; samplerIndex < TextureCubeGlobal::samplersToReleaseAtExit.size(); ++samplerIndex)
    {
        vkDestroySampler( GfxDeviceGlobal::device, TextureCubeGlobal::samplersToReleaseAtExit[ samplerIndex ], nullptr );
    }

    for (std::size_t imageIndex = 0; imageIndex < TextureCubeGlobal::imagesToReleaseAtExit.size(); ++imageIndex)
    {
        vkDestroyImage( GfxDeviceGlobal::device, TextureCubeGlobal::imagesToReleaseAtExit[ imageIndex ], nullptr );
    }

    for (std::size_t imageViewIndex = 0; imageViewIndex < TextureCubeGlobal::imageViewsToReleaseAtExit.size(); ++imageViewIndex)
    {
        vkDestroyImageView( GfxDeviceGlobal::device, TextureCubeGlobal::imageViewsToReleaseAtExit[ imageViewIndex ], nullptr );
    }

    for (std::size_t memoryIndex = 0; memoryIndex < TextureCubeGlobal::memoryToReleaseAtExit.size(); ++memoryIndex)
    {
        vkFreeMemory( GfxDeviceGlobal::device, TextureCubeGlobal::memoryToReleaseAtExit[ memoryIndex ], nullptr );
    }

    for (std::size_t bufferIndex = 0; bufferIndex < TextureCubeGlobal::buffersToReleaseAtExit.size(); ++bufferIndex)
    {
        vkDestroyBuffer( GfxDeviceGlobal::device, TextureCubeGlobal::buffersToReleaseAtExit[ bufferIndex ], nullptr );
    }
}

ae3d::TextureCube* ae3d::TextureCube::GetDefaultTexture()
{
    if (TextureCubeGlobal::defaultTexture.view == VK_NULL_HANDLE)
    {
        std::uint8_t imageData[ 32 * 32 * 4 ];

        for (int i = 0; i < 32 * 32 * 4; ++i)
        {
            imageData[ i ] = 0xFF;
        }

        // FIXME: This is a hack, implement LoadFromData instead
        TextureCubeGlobal::defaultTexture.Load( FileSystem::FileContents( "test_dxt1.dds" ), FileSystem::FileContents( "test_dxt1.dds" ),
            FileSystem::FileContents( "test_dxt1.dds" ), FileSystem::FileContents( "test_dxt1.dds" ),
            FileSystem::FileContents( "test_dxt1.dds" ), FileSystem::FileContents( "test_dxt1.dds" ), TextureWrap::Clamp, TextureFilter::Linear, Mipmaps::None, ColorSpace::SRGB );
    }

    return &TextureCubeGlobal::defaultTexture;
}

void ae3d::TextureCube::Load( const FileSystem::FileContentsData& negX, const FileSystem::FileContentsData& posX,
                              const FileSystem::FileContentsData& negY, const FileSystem::FileContentsData& posY,
                              const FileSystem::FileContentsData& negZ, const FileSystem::FileContentsData& posZ,
                              TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    colorSpace = aColorSpace;

    posXpath = posX.path;
    posYpath = posY.path;
    posZpath = posZ.path;
    negXpath = negX.path;
    negYpath = negY.path;
    negZpath = negZ.path;

    const std::string paths[] = { posX.path, negX.path, negY.path, posY.path, negZ.path, posZ.path };
    const std::vector< unsigned char >* datas[] = { &posX.data, &negX.data, &negY.data, &posY.data, &negZ.data, &posZ.data };

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;    
    VkImage images[ 6 ];
    VkDeviceMemory deviceMemories[ 6 ];

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
 
    VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::texCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer in TextureCube" );

    DDSLoader::Output ddsOutput[ 6 ];
    bool isSomeFaceDDS = false;

    for (int face = 0; face < 6; ++face)
    {
        const bool isDDS = paths[ face ].find( ".dds" ) != std::string::npos || paths[ face ].find( ".DDS" ) != std::string::npos;

        if (HasStbExtension( paths[ face ] ))
        {
            int components;
            unsigned char* data = stbi_load_from_memory( datas[ face ]->data(), static_cast<int>(datas[ face ]->size()), &width, &height, &components, 4 );

            if (data == nullptr)
            {
                const std::string reason( stbi_failure_reason() );
                System::Print( "%s failed to load. stb_image's reason: %s\n", paths[ face ].c_str(), reason.c_str() );
                return;
            }

            if (static_cast< int >( GfxDeviceGlobal::properties.limits.maxImageDimensionCube) < width ||
                static_cast< int >( GfxDeviceGlobal::properties.limits.maxImageDimensionCube) < height)
            {
                System::Print( "%s is too big (%dx%d), max supported size is %dx%d.\n", paths[ face ].c_str(), width, height,
                    GfxDeviceGlobal::properties.limits.maxImageDimensionCube, GfxDeviceGlobal::properties.limits.maxImageDimensionCube );
                width = GfxDeviceGlobal::properties.limits.maxImageDimensionCube;
                height = GfxDeviceGlobal::properties.limits.maxImageDimensionCube;
            }

            opaque = (components == 3 || components == 1);
            mipLevelCount = mipmaps == Mipmaps::None ? 1 : MathUtil::GetMipmapCount( width, height );

            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.extent = { (std::uint32_t)width, (std::uint32_t)height, 1 };
            imageCreateInfo.format = format;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
            imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

            err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &images[ face ] );
            AE3D_CHECK_VULKAN( err, "vkCreateImage" );
            TextureCubeGlobal::imagesToReleaseAtExit.push_back( images[ face ] );

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements( GfxDeviceGlobal::device, images[ face ], &memReqs );
            VkMemoryAllocateInfo memAllocInfo = {};
            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAllocInfo.pNext = nullptr;
            memAllocInfo.allocationSize = memReqs.size;
            memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

            err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemories[ face ] );
            AE3D_CHECK_VULKAN( err, "vkAllocateMemory in TextureCube" );
            TextureCubeGlobal::memoryToReleaseAtExit.push_back( deviceMemories[ face ] );
            Statistics::IncAllocCalls();
            Statistics::IncTotalAllocCalls();

            err = vkBindImageMemory( GfxDeviceGlobal::device, images[ face ], deviceMemories[ face ], 0 );
            AE3D_CHECK_VULKAN( err, "vkBindImageMemory in TextureCube" );

            VkImageSubresource subRes = {};
            subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subRes.mipLevel = 0;
            subRes.arrayLayer = 0;

            VkSubresourceLayout subResLayout;
            vkGetImageSubresourceLayout( GfxDeviceGlobal::device, images[ face ], &subRes, &subResLayout );

            void* mapped;
            err = vkMapMemory( GfxDeviceGlobal::device, deviceMemories[ face ], 0, memReqs.size, 0, &mapped );
            AE3D_CHECK_VULKAN( err, "vkMapMemory in TextureCube" );

            const int bytesPerPixel = 4;

            if (MathUtil::IsPowerOfTwo( width ) && MathUtil::IsPowerOfTwo( height ))
            {
                std::memcpy( mapped, data, width * height * bytesPerPixel );
            }
            else
            {
                const std::size_t rowSize = bytesPerPixel * width;
                char* mappedPos = (char*)mapped;
                char* dataPos = (char*)data;

                for (int i = 0; i < height; ++i)
                {
                    std::memcpy( mappedPos, dataPos, rowSize );
                    mappedPos += subResLayout.rowPitch;
                    dataPos += rowSize;
                }
            }

            VkMappedMemoryRange flushRange = {};
            flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            flushRange.memory = deviceMemories[ face ];
            flushRange.offset = 0;
            flushRange.size = width * height * bytesPerPixel;
            vkFlushMappedMemoryRanges( GfxDeviceGlobal::device, 1, &flushRange );

            vkUnmapMemory( GfxDeviceGlobal::device, deviceMemories[ face ] );

            stbi_image_free( data );

            VkImageSubresourceRange range = {};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = 0;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.image = images[ face ];
            imageMemoryBarrier.subresourceRange = range;

            vkCmdPipelineBarrier(
                                 GfxDeviceGlobal::texCmdBuffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &imageMemoryBarrier );            
        }
        else if (isDDS && GfxDeviceGlobal::deviceFeatures.textureCompressionBC)
        {
            isSomeFaceDDS = true;
            auto fileContents = FileSystem::FileContents( paths[ face ].c_str() );
            const DDSLoader::LoadResult loadResult = DDSLoader::Load( fileContents, width, height, opaque, ddsOutput[ face ] );

            if (loadResult != DDSLoader::LoadResult::Success)
            {
                ae3d::System::Print( "DDS Loader could not load %s", paths[ face ].c_str() );
                return;
            }

            if (static_cast< int >( GfxDeviceGlobal::properties.limits.maxImageDimensionCube) < width ||
                static_cast< int >( GfxDeviceGlobal::properties.limits.maxImageDimensionCube) < height)
            {
                System::Print( "%s is too big (%dx%d), max supported size is %dx%d.\n", paths[ face ].c_str(), width, height,
                    GfxDeviceGlobal::properties.limits.maxImageDimensionCube, GfxDeviceGlobal::properties.limits.maxImageDimensionCube );
                width = GfxDeviceGlobal::properties.limits.maxImageDimensionCube;
                height = GfxDeviceGlobal::properties.limits.maxImageDimensionCube;
            }

            int bytesPerPixel = 1;

            mipLevelCount = mipmaps == Mipmaps::None ? 1 : ddsOutput[ face ].dataOffsets.count;
        
            if (!opaque)
            {
               format = (colorSpace == ColorSpace::RGB) ? VK_FORMAT_BC1_RGBA_UNORM_BLOCK : VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            }

            if (ddsOutput[ face ].format == DDSLoader::Format::BC1)
            {
               format = (colorSpace == ColorSpace::RGB) ? VK_FORMAT_BC1_RGBA_UNORM_BLOCK : VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            }
            else if (ddsOutput[ face ].format == DDSLoader::Format::BC2)
            {
                format = (colorSpace == ColorSpace::RGB) ? VK_FORMAT_BC2_UNORM_BLOCK : VK_FORMAT_BC2_SRGB_BLOCK;
                bytesPerPixel = 2;
            }
            else if (ddsOutput[ face ].format == DDSLoader::Format::BC3)
            {
                format = (colorSpace == ColorSpace::RGB) ? VK_FORMAT_BC3_UNORM_BLOCK : VK_FORMAT_BC3_SRGB_BLOCK;
                bytesPerPixel = 2;
            }
            else if (ddsOutput[ face ].format == DDSLoader::Format::BC4U)
            {
                format = VK_FORMAT_BC4_UNORM_BLOCK;
            }
            else if (ddsOutput[ face ].format == DDSLoader::Format::BC4S)
            {
                format = VK_FORMAT_BC4_SNORM_BLOCK;
            }
            else if (ddsOutput[ face ].format == DDSLoader::Format::BC5U)
            {
                format = VK_FORMAT_BC5_UNORM_BLOCK;
                bytesPerPixel = 2;
            }
            else if (ddsOutput[ face ].format == DDSLoader::Format::BC5S)
            {
                format = VK_FORMAT_BC5_SNORM_BLOCK;
                bytesPerPixel = 2;
            }
            else
            {
                ae3d::System::Assert( false, "DDS reader error: Unhandled DDS format!" );
            }

            ae3d::System::Assert( ddsOutput[face ].dataOffsets.count > 0, "DDS reader error: dataoffsets is empty" );

            VkImageCreateInfo imageCreateInfo = {};
            imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCreateInfo.arrayLayers = 1;
            imageCreateInfo.extent = { (std::uint32_t)width, (std::uint32_t)height, 1 };
            imageCreateInfo.format = format;
            imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
            imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            imageCreateInfo.mipLevels = 1;
            imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
            imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

            err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &images[ face ] );
            AE3D_CHECK_VULKAN( err, "vkCreateImage" );
            TextureCubeGlobal::imagesToReleaseAtExit.push_back( images[ face ] );

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements( GfxDeviceGlobal::device, images[ face ], &memReqs );

            VkMemoryAllocateInfo memAllocInfo = {};
            memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memAllocInfo.pNext = nullptr;
            memAllocInfo.allocationSize = memReqs.size;
            memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );

            err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemories[ face ] );
            AE3D_CHECK_VULKAN( err, "vkAllocateMemory in TextureCube" );
            TextureCubeGlobal::memoryToReleaseAtExit.push_back( deviceMemories[ face ] );
            Statistics::IncAllocCalls();
            Statistics::IncTotalAllocCalls();

            err = vkBindImageMemory( GfxDeviceGlobal::device, images[ face ], deviceMemories[ face ], 0 );
            AE3D_CHECK_VULKAN( err, "vkBindImageMemory in TextureCube" );

            VkImageSubresource subRes = {};
            subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subRes.mipLevel = 0;
            subRes.arrayLayer = 0;

            VkSubresourceLayout subResLayout;
            vkGetImageSubresourceLayout( GfxDeviceGlobal::device, images[ face ], &subRes, &subResLayout );

            void* mapped;
            err = vkMapMemory( GfxDeviceGlobal::device, deviceMemories[ face ], 0, memReqs.size, 0, &mapped );
            AE3D_CHECK_VULKAN( err, "vkMapMemory in TextureCube" );

            const size_t amountToCopy = unsigned(width * height * bytesPerPixel) < memReqs.size ? unsigned(width * height * bytesPerPixel) : memReqs.size;

            std::memcpy( mapped, &ddsOutput[ face ].imageData[ ddsOutput[ face ].dataOffsets[ 0 ] ], amountToCopy );

            VkMappedMemoryRange flushRange = {};
            flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            flushRange.memory = deviceMemories[ face ];
            flushRange.offset = 0;
            flushRange.size = memReqs.size;
            vkFlushMappedMemoryRanges( GfxDeviceGlobal::device, 1, &flushRange );

            vkUnmapMemory( GfxDeviceGlobal::device, deviceMemories[ face ] );

            SetImageLayout( GfxDeviceGlobal::texCmdBuffer,
                images[ face ],
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, 0, 1 );
        }
        else
        {
            System::Print( "Unknown/unsupported texture file extension: %s\n", paths[ face ].c_str() );
        }
    }

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.arrayLayers = 6;
    imageCreateInfo.extent = { (std::uint32_t)width, (std::uint32_t)height, 1 };
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageCreateInfo.format = format;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.mipLevels = mipLevelCount;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &image );
    AE3D_CHECK_VULKAN( err, "vkCreateImage in TextureCube" );

    TextureCubeGlobal::imagesToReleaseAtExit.push_back( image );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)image, VK_OBJECT_TYPE_IMAGE, paths[ 0 ].c_str() );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, image, &memReqs );
    
    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory in TextureCube" );
    TextureCubeGlobal::memoryToReleaseAtExit.push_back( deviceMemory );
    Statistics::IncAllocCalls();
    Statistics::IncTotalAllocCalls();

    err = vkBindImageMemory( GfxDeviceGlobal::device, image, deviceMemory, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindImageMemory in TextureCube" );

    SetImageLayout( GfxDeviceGlobal::texCmdBuffer,
        image,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, 0, 1 );

    for (int face = 0; face < 6; ++face)
    {
        VkImageCopy copyRegion = {};

        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.baseArrayLayer = 0;
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = { 0, 0, 0 };

        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.baseArrayLayer = face;
        copyRegion.dstSubresource.mipLevel = 0;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = { 0, 0, 0 };

        copyRegion.extent.width = width;
        copyRegion.extent.height = height;
        copyRegion.extent.depth = 1;

        vkCmdCopyImage( GfxDeviceGlobal::texCmdBuffer,
            images[ face ], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copyRegion );
    }

    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 6;

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = range;

    vkCmdPipelineBarrier(
                         GfxDeviceGlobal::texCmdBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &imageMemoryBarrier );

    if (mipLevelCount > 1)
    {
        for (int mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel)
        {
            SetImageLayout( GfxDeviceGlobal::texCmdBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, mipLevel, 1 );            
        }
    }

    err = vkEndCommandBuffer( GfxDeviceGlobal::texCmdBuffer );
    AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer in TextureCube" );

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::texCmdBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit in TextureCube" );

    vkDeviceWaitIdle( GfxDeviceGlobal::device );

    err = vkBeginCommandBuffer( GfxDeviceGlobal::texCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer in TextureCube" );

    for (int face = 0; face < 6; ++face)
    {
        Array< VkBuffer > stagingBuffers( mipLevelCount );
        Array< VkDeviceMemory > stagingMemory( mipLevelCount );

        for (int mipLevel = 1; mipLevel < mipLevelCount; ++mipLevel)
        {
            if (isSomeFaceDDS)
            {
                const std::int32_t mipWidth = MathUtil::Max( width >> mipLevel, 1 );
                const std::int32_t mipHeight = MathUtil::Max( height >> mipLevel, 1 );

                const VkDeviceSize bc1BlockSize = opaque ? 8 : 16;
                VkDeviceSize imageSize = (mipWidth / 4) * (mipHeight / 4) * (format == VK_FORMAT_BC5_UNORM_BLOCK ? 16 : bc1BlockSize);

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
                err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferCreateInfo, nullptr, &stagingBuffers[ mipLevel ] );
                AE3D_CHECK_VULKAN( err, "vkCreateBuffer staging" );
                TextureCubeGlobal::buffersToReleaseAtExit.push_back( stagingBuffers[ mipLevel ] );

                vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, stagingBuffers[ mipLevel ], &memReqs );

                memAllocInfo.allocationSize = memReqs.size;
                memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
                err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &stagingMemory[ mipLevel ] );
                AE3D_CHECK_VULKAN( err, "vkAllocateMemory" );
                TextureCubeGlobal::memoryToReleaseAtExit.push_back( stagingMemory[ mipLevel ] );

                err = vkBindBufferMemory( GfxDeviceGlobal::device, stagingBuffers[ mipLevel ], stagingMemory[ mipLevel ], 0 );
                AE3D_CHECK_VULKAN( err, "vkBindBufferMemory staging" );

                void* stagingData;
                err = vkMapMemory( GfxDeviceGlobal::device, stagingMemory[ mipLevel ], 0, memReqs.size, 0, &stagingData );
                AE3D_CHECK_VULKAN( err, "vkMapMemory in Texture2D" );

                VkDeviceSize amountToCopy = imageSize;
                if (ddsOutput[ face ].dataOffsets[ mipLevel ] + imageSize >= ddsOutput[ face ].imageData.count)
                {
                    amountToCopy = ddsOutput[ face ].imageData.count - ddsOutput[ face ].dataOffsets[ mipLevel ];
                }

                std::memcpy( stagingData, &ddsOutput[ face ].imageData[ ddsOutput[ face ].dataOffsets[ mipLevel ] ], amountToCopy );

                VkBufferImageCopy bufferCopyRegion = {};
                bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                bufferCopyRegion.imageSubresource.mipLevel = mipLevel;
                bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                bufferCopyRegion.imageSubresource.layerCount = 1;
                bufferCopyRegion.imageExtent.width = mipWidth;
                bufferCopyRegion.imageExtent.height = mipHeight;
                bufferCopyRegion.imageExtent.depth = 1;
                bufferCopyRegion.bufferOffset = 0;

                vkCmdCopyBufferToImage( GfxDeviceGlobal::texCmdBuffer, stagingBuffers[ mipLevel ], image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion );
            }
            else
            {
                const std::int32_t mipWidth = MathUtil::Max( width >> mipLevel, 1 );
                const std::int32_t mipHeight = MathUtil::Max( height >> mipLevel, 1 );

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
                imageBlit.dstSubresource.mipLevel = mipLevel;
                imageBlit.dstOffsets[ 0 ] = { 0, 0, 0 };
                imageBlit.dstOffsets[ 1 ] = { mipWidth, mipHeight, 1 };

                vkCmdBlitImage( GfxDeviceGlobal::texCmdBuffer, image, VK_IMAGE_LAYOUT_GENERAL, image,
                    VK_IMAGE_LAYOUT_GENERAL, 1, &imageBlit, VK_FILTER_LINEAR );
            }
        }
    }

    if (mipLevelCount > 1 && isSomeFaceDDS)
    {
        for (int mipLevel = 0; mipLevel < mipLevelCount; ++mipLevel)
        {
            SetImageLayout( GfxDeviceGlobal::texCmdBuffer, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6, mipLevel, 1 );
        }
    }
    
    err = vkEndCommandBuffer( GfxDeviceGlobal::texCmdBuffer );
    AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer in TextureCube" );

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::texCmdBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit in TextureCube" );

    vkDeviceWaitIdle( GfxDeviceGlobal::device );

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = imageCreateInfo.format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.image = image;
    err = vkCreateImageView( GfxDeviceGlobal::device, &viewInfo, nullptr, &view );
    AE3D_CHECK_VULKAN( err, "vkCreateImageView in TextureCube" );
    TextureCubeGlobal::imageViewsToReleaseAtExit.push_back( view );

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter == ae3d::TextureFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerInfo.minFilter = samplerInfo.magFilter;
    samplerInfo.mipmapMode = filter == ae3d::TextureFilter::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = wrap == ae3d::TextureWrap::Repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.mipLodBias = 0;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = static_cast< float >( mipLevelCount );
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    err = vkCreateSampler( GfxDeviceGlobal::device, &samplerInfo, nullptr, &sampler );
    AE3D_CHECK_VULKAN( err, "vkCreateSampler" );
    TextureCubeGlobal::samplersToReleaseAtExit.push_back( sampler );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)sampler, VK_OBJECT_TYPE_SAMPLER, "sampler" );
}
