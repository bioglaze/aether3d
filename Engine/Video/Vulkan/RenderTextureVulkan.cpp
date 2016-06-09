#include "RenderTexture.hpp"
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "VulkanUtils.hpp"

namespace ae3d
{
    void GetMemoryType( std::uint32_t typeBits, VkFlags properties, std::uint32_t* typeIndex ); // Defined in GfxDeviceVulkan.cpp 
    void AllocateSetupCommandBuffer();
    void FlushSetupCommandBuffer();
}

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkCommandBuffer setupCmdBuffer;
    extern VkRenderPass renderPass;
}

void ae3d::RenderTexture::Create2D( int aWidth, int aHeight, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter )
{
    if (aWidth <= 0 || aHeight <= 0)
    {
        System::Print( "Render texture has invalid dimension!\n" );
        return;
    }
    
    width = aWidth;
    height = aHeight;
    wrap = aWrap;
    filter = aFilter;
    isCube = false;
    isRenderTexture = true;
    dataType = aDataType;

    // Color

    const VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

    VkImageCreateInfo colorImage = {};
    colorImage.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    colorImage.imageType = VK_IMAGE_TYPE_2D;
    colorImage.format = colorFormat;
    colorImage.extent.width = width;
    colorImage.extent.height = height;
    colorImage.extent.depth = 1;
    colorImage.mipLevels = 1;
    colorImage.arrayLayers = 1;
    colorImage.samples = VK_SAMPLE_COUNT_1_BIT;
    colorImage.tiling = VK_IMAGE_TILING_OPTIMAL;
    colorImage.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    colorImage.flags = 0;

    VkImageViewCreateInfo colorImageView = {};
    colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageView.format = colorFormat;
    colorImageView.flags = 0;
    colorImageView.subresourceRange = {};
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 1;

    VkResult err = vkCreateImage( GfxDeviceGlobal::device, &colorImage, nullptr, &color.image );
    AE3D_CHECK_VULKAN( err, "render texture 2d color image" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "render texture 2d color" );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, color.image, &memReqs );

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize = memReqs.size;

    GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &color.mem );
    AE3D_CHECK_VULKAN( err, "render texture 2d color image memory" );

    err = vkBindImageMemory( GfxDeviceGlobal::device, color.image, color.mem, 0 );
    AE3D_CHECK_VULKAN( err, "render texture 2d color image bind memory" );

    AllocateSetupCommandBuffer();

    SetImageLayout( GfxDeviceGlobal::setupCmdBuffer,
        color.image,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 0, 1 );
    
    colorImageView.image = color.image;
    err = vkCreateImageView( GfxDeviceGlobal::device, &colorImageView, nullptr, &color.view );
    AE3D_CHECK_VULKAN( err, "render texture 2d color image view" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, "render texture 2d color view" );

    // Depth/Stencil

    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    VkImageCreateInfo depthImage = colorImage;
    depthImage.format = depthFormat;
    depthImage.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageViewCreateInfo depthStencilView = {};
    depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthStencilView.format = depthFormat;
    depthStencilView.flags = 0;
    depthStencilView.subresourceRange = {};
    depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthStencilView.subresourceRange.baseMipLevel = 0;
    depthStencilView.subresourceRange.levelCount = 1;
    depthStencilView.subresourceRange.baseArrayLayer = 0;
    depthStencilView.subresourceRange.layerCount = 1;

    err = vkCreateImage( GfxDeviceGlobal::device, &depthImage, nullptr, &depth.image );
    AE3D_CHECK_VULKAN( err, "render texture 2d depth image" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "render texture 2d depth" );

    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, depth.image, &memReqs );
    memAlloc.allocationSize = memReqs.size;
    GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &depth.mem );
    AE3D_CHECK_VULKAN( err, "render texture 2d depth memory" );

    err = vkBindImageMemory( GfxDeviceGlobal::device, depth.image, depth.mem, 0 );
    AE3D_CHECK_VULKAN( err, "render texture 2d depth bind" );

    SetImageLayout( GfxDeviceGlobal::setupCmdBuffer,
        depth.image,
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 0, 1 );

    depthStencilView.image = depth.image;
    err = vkCreateImageView( GfxDeviceGlobal::device, &depthStencilView, nullptr, &depth.view );
    AE3D_CHECK_VULKAN( err, "render texture 2d depth view" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, "render texture 2d depth view" );

    FlushSetupCommandBuffer();

    VkImageView attachments[ 2 ];
    attachments[ 0 ] = color.view;
    attachments[ 1 ] = depth.view;

    VkFramebufferCreateInfo fbufCreateInfo = {};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.renderPass = GfxDeviceGlobal::renderPass;
    fbufCreateInfo.attachmentCount = 2;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = width;
    fbufCreateInfo.height = height;
    fbufCreateInfo.layers = 1;

    err = vkCreateFramebuffer( GfxDeviceGlobal::device, &fbufCreateInfo, nullptr, &frameBuffer );
    AE3D_CHECK_VULKAN( err, "rendertexture framebuffer" );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)image, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, "render texture 2d framebuffer" );
}

void ae3d::RenderTexture::CreateCube( int aDimension, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter )
{
    if (aDimension <= 0)
    {
        System::Print( "Render texture has invalid dimension!\n" );
        return;
    }
    
    width = height = aDimension;
    wrap = aWrap;
    filter = aFilter;
    isCube = true;
    isRenderTexture = true;
    dataType = aDataType;
}