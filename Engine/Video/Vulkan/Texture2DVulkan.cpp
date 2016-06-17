#include "Texture2D.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
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
}

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace, float aAnisotropy )
{
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

    if (!fileContents.isLoaded)
    {
        *this = Texture2DGlobal::defaultTexture;
        return;
    }

    if (HasStbExtension( fileContents.path ))
    {
        LoadSTB( fileContents );
    }
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

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.memoryTypeIndex = 0;
    memAllocInfo.allocationSize = 0;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = width * height * 4;
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
    std::memcpy( stagingData, data, width * height * 4 );
    vkUnmapMemory( GfxDeviceGlobal::device, stagingMemory );

    std::vector<VkBufferImageCopy> bufferCopyRegions;
    std::uint32_t offset = 0;

    const auto format = VK_FORMAT_R8G8B8A8_UNORM;
    const int mipLevels = mipmaps == Mipmaps::Generate ? static_cast<int>(MathUtil::GetMipmapCount( width, height )) : 1;

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

        offset += width * height * 4;
    }

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageCreateInfo.extent = { static_cast<std::uint32_t>( width ), static_cast<std::uint32_t>( height ), 1 };
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &image );
    AE3D_CHECK_VULKAN( err, "vkCreateImage" );

    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, image, &memReqs );

    memAllocInfo.allocationSize = memReqs.size;
    GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex );

    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory" );

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
        mipLevels );

    vkCmdCopyBufferToImage(
        Texture2DGlobal::texCmdBuffer,
        stagingBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<std::uint32_t>( bufferCopyRegions.size() ),
        bufferCopyRegions.data()
    );

    for (int i = 1; i < mipLevels; ++i)
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
        mipLevels );

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
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.image = image;
    err = vkCreateImageView( GfxDeviceGlobal::device, &viewInfo, nullptr, &view );
    AE3D_CHECK_VULKAN( err, "vkCreateImageView in Texture2D" );

    stbi_image_free( data );
}

ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    Texture2DGlobal::defaultTexture.handle = 1;
    Texture2DGlobal::defaultTexture.width = 256;
    Texture2DGlobal::defaultTexture.height = 256;
    return &Texture2DGlobal::defaultTexture;
}
