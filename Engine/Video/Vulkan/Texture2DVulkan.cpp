#include "Texture2D.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "FileSystem.hpp"
#include "System.hpp"

bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp
void Tokenize( const std::string& str,
    std::vector< std::string >& tokens,
    const std::string& delimiters = " " ); // Defined in TextureCommon.cpp

namespace ae3d
{
    void CheckVulkanResult( VkResult result, const char* message ); // Defined in GfxDeviceVulkan.cpp 
    VkBool32 GetMemoryType( std::uint32_t typeBits, VkFlags properties, std::uint32_t* typeIndex ); // Defined in GfxDeviceVulkan.cpp 
    void SetImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout );
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
        CheckVulkanResult( err, "vkAllocateCommandBuffers Texture2D" );
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

    //const VkFormat format = opaque ? VK_FORMAT_R8G8B8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
    const VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = { (std::uint32_t)width, (std::uint32_t)height, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.flags = 0;

    VkImage mappableImage;
    VkResult err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &mappableImage );
    CheckVulkanResult( err, "vkCreateImage" );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, mappableImage, &memReqs );

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.memoryTypeIndex = 0;
    memAllocInfo.allocationSize = memReqs.size;

    GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex );

    VkDeviceMemory mappableMemory;
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &mappableMemory );
    CheckVulkanResult( err, "vkAllocateMemory in Texture2D" );

    err = vkBindImageMemory( GfxDeviceGlobal::device, mappableImage, mappableMemory, 0 );
    CheckVulkanResult( err, "vkBindImageMemory in Texture2D" );

    VkImageSubresource subRes = {};
    subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subRes.mipLevel = 0;
    subRes.arrayLayer = 0;

    VkSubresourceLayout subResLayout;
    vkGetImageSubresourceLayout( GfxDeviceGlobal::device, mappableImage, &subRes, &subResLayout );

    void* mapped;
    err = vkMapMemory( GfxDeviceGlobal::device, mappableMemory, 0, memReqs.size, 0, &mapped );
    CheckVulkanResult( err, "vkMapMemory in Texture2D" );

    const int bytesPerPixel = 4;
    std::size_t dataSize = bytesPerPixel * width * height;
    std::memcpy( mapped, data, dataSize );

    vkUnmapMemory( GfxDeviceGlobal::device, mappableMemory );

    // Staging (as opposed to linear loading path)
    {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufInfo.pNext = nullptr;
        cmdBufInfo.pInheritanceInfo = nullptr;
        cmdBufInfo.flags = 0;

        err = vkBeginCommandBuffer( Texture2DGlobal::texCmdBuffer, &cmdBufInfo );
        CheckVulkanResult( err, "vkBeginCommandBuffer in Texture2D" );

        SetImageLayout( Texture2DGlobal::texCmdBuffer,
            mappableImage,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_GENERAL, // Was UNDEFINED in sample code, but caused a warning.
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL );

        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        err = vkCreateImage( GfxDeviceGlobal::device, &imageCreateInfo, nullptr, &image );
        CheckVulkanResult( err, "vkCreateImage in Texture2D" );

        vkGetImageMemoryRequirements( GfxDeviceGlobal::device, image, &memReqs );

        memAllocInfo.allocationSize = memReqs.size;

        GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAllocInfo.memoryTypeIndex );

        err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &deviceMemory );
        CheckVulkanResult( err, "vkAllocateMemory in Texture2D" );

        err = vkBindImageMemory( GfxDeviceGlobal::device, image, deviceMemory, 0 );
        CheckVulkanResult( err, "vkBindImageMemory in Texture2D" );

        SetImageLayout( Texture2DGlobal::texCmdBuffer,
            image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

        VkImageCopy copyRegion = {};

        copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.srcSubresource.baseArrayLayer = 0;
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcOffset = { 0, 0, 0 };

        copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.dstSubresource.baseArrayLayer = 0;
        copyRegion.dstSubresource.mipLevel = 0;
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstOffset = { 0, 0, 0 };

        copyRegion.extent.width = width;
        copyRegion.extent.height = height;
        copyRegion.extent.depth = 1;

        vkCmdCopyImage( Texture2DGlobal::texCmdBuffer,
            mappableImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &copyRegion );

        SetImageLayout( Texture2DGlobal::texCmdBuffer,
            image,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

        err = vkEndCommandBuffer( Texture2DGlobal::texCmdBuffer );
        CheckVulkanResult( err, "vkEndCommandBuffer in Texture2D" );

        VkFence nullFence = { VK_NULL_HANDLE };

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &Texture2DGlobal::texCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, nullFence );
        CheckVulkanResult( err, "vkQueueSubmit in Texture2D" );

        err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
        CheckVulkanResult( err, "vkQueueWaitIdle in Texture2D" );
    }

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
    CheckVulkanResult( err, "vkCreateImageView in Texture2D" );

    stbi_image_free( data );
}

const ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    Texture2DGlobal::defaultTexture.handle = 1;
    Texture2DGlobal::defaultTexture.width = 256;
    Texture2DGlobal::defaultTexture.height = 256;
    return &Texture2DGlobal::defaultTexture;
}
