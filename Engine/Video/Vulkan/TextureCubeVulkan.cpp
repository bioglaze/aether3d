#include "TextureCube.hpp"
#include <cstring>
#include <vulkan/vulkan.h>
#include "stb_image.c"
#include "FileSystem.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "VulkanUtils.hpp"

bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp

namespace ae3d
{
    void GetMemoryType( std::uint32_t typeBits, VkFlags properties, std::uint32_t* typeIndex ); // Defined in GfxDeviceVulkan.cpp 
}

namespace MathUtil
{
    bool IsPowerOfTwo( unsigned i );
}

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkPhysicalDevice physicalDevice;
    extern VkQueue graphicsQueue;
    extern VkCommandPool cmdPool;
}

namespace TextureCubeGlobal
{
    ae3d::TextureCube defaultTexture;
    VkCommandBuffer texCmdBuffer = VK_NULL_HANDLE;
    std::vector< VkSampler > samplersToReleaseAtExit;
    std::vector< VkImage > imagesToReleaseAtExit;
    std::vector< VkImageView > imageViewsToReleaseAtExit;
    std::vector< VkDeviceMemory > memoryToReleaseAtExit;
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
}

void ae3d::TextureCube::Load( const FileSystem::FileContentsData& negX, const FileSystem::FileContentsData& posX,
                              const FileSystem::FileContentsData& negY, const FileSystem::FileContentsData& posY,
                              const FileSystem::FileContentsData& negZ, const FileSystem::FileContentsData& posZ,
                              TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace )
{
    if (TextureCubeGlobal::texCmdBuffer == VK_NULL_HANDLE)
    {
        System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );
        System::Assert( GfxDeviceGlobal::cmdPool != VK_NULL_HANDLE, "cmdPool not initialized" );

        VkCommandBufferAllocateInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufInfo.commandPool = GfxDeviceGlobal::cmdPool;
        cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufInfo.commandBufferCount = 1;

        VkResult err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &cmdBufInfo, &TextureCubeGlobal::texCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers TextureCube" );
    }

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

    const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageCreateInfo.flags = 0;
    
    VkImage images[ 6 ];
    VkDeviceMemory deviceMemories[ 6 ];

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.memoryTypeIndex = 0;
    memAllocInfo.allocationSize = 0;

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;
    cmdBufInfo.pInheritanceInfo = nullptr;
    cmdBufInfo.flags = 0;

    VkResult err = vkBeginCommandBuffer( TextureCubeGlobal::texCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer in TextureCube" );

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

            opaque = (components == 3 || components == 1);
            imageCreateInfo.extent = { (std::uint32_t)width, (std::uint32_t)height, 1 };

            err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &images[ face ] );
            AE3D_CHECK_VULKAN( err, "vkCreateImage" );
            TextureCubeGlobal::imagesToReleaseAtExit.push_back( images[ face ] );

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements( GfxDeviceGlobal::device, images[ face ], &memReqs );
            memAllocInfo.allocationSize = memReqs.size;

            GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex );

            err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemories[ face ] );
            AE3D_CHECK_VULKAN( err, "vkAllocateMemory in TextureCube" );
            TextureCubeGlobal::memoryToReleaseAtExit.push_back( deviceMemories[ face ] );

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

            vkUnmapMemory( GfxDeviceGlobal::device, deviceMemories[ face ] );

            stbi_image_free( data );

            SetImageLayout( TextureCubeGlobal::texCmdBuffer,
                images[ face ],
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,//VK_IMAGE_LAYOUT_PREINITIALIZED,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, 0, 1 );
        }
        else if (isDDS)
        {
            //LoadDDS( fileContents.path.c_str() );
        }
    }
 
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageCreateInfo.arrayLayers = 6;

    err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &image );
    AE3D_CHECK_VULKAN( err, "vkCreateImage in TextureCube" );
    TextureCubeGlobal::imagesToReleaseAtExit.push_back( image );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, paths[ 0 ].c_str() );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, image, &memReqs );
    memAllocInfo.allocationSize = memReqs.size;

    GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory in TextureCube" );
    TextureCubeGlobal::memoryToReleaseAtExit.push_back( deviceMemory );

    err = vkBindImageMemory( GfxDeviceGlobal::device, image, deviceMemory, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindImageMemory in TextureCube" );

    SetImageLayout( TextureCubeGlobal::texCmdBuffer,
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

        vkCmdCopyImage( TextureCubeGlobal::texCmdBuffer,
            images[ face ], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copyRegion );
    }

    SetImageLayout( TextureCubeGlobal::texCmdBuffer,
        image,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6, 0, 1 );

    err = vkEndCommandBuffer( TextureCubeGlobal::texCmdBuffer );
    AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer in TextureCube" );

    VkFence nullFence = { VK_NULL_HANDLE };

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &TextureCubeGlobal::texCmdBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, nullFence );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit in TextureCube" );

    err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
    AE3D_CHECK_VULKAN( err, "vkQueueWaitIdle in TextureCube" );

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext = nullptr;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
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
    samplerInfo.maxLod = static_cast< float >( mipLevelCount );
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    err = vkCreateSampler( GfxDeviceGlobal::device, &samplerInfo, nullptr, &sampler );
    AE3D_CHECK_VULKAN( err, "vkCreateSampler" );
    TextureCubeGlobal::samplersToReleaseAtExit.push_back( sampler );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, "sampler" );
}
