#include "RenderTexture.hpp"
#include <vector>
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "Statistics.hpp"
#include "VulkanUtils.hpp"

namespace ae3d
{
    void AllocateSetupCommandBuffer();
    void FlushSetupCommandBuffer();
}

namespace GfxDeviceGlobal
{
    extern VkDevice device;
    extern VkCommandBuffer setupCmdBuffer;
    extern VkFormat colorFormat;
    extern VkFormat depthFormat;
}

namespace RenderTextureGlobal
{
    std::vector< VkSampler > samplersToReleaseAtExit;
    std::vector< VkImage > imagesToReleaseAtExit;
    std::vector< VkImageView > imageViewsToReleaseAtExit;
    std::vector< VkDeviceMemory > memoryToReleaseAtExit;
    std::vector< VkFramebuffer > fbsToReleaseAtExit;
    std::vector< VkRenderPass > renderPassesToReleaseAtExit;
}

void ae3d::RenderTexture::DestroyTextures()
{
    for (std::size_t samplerIndex = 0; samplerIndex < RenderTextureGlobal::samplersToReleaseAtExit.size(); ++samplerIndex)
    {
        vkDestroySampler( GfxDeviceGlobal::device, RenderTextureGlobal::samplersToReleaseAtExit[ samplerIndex ], nullptr );
    }

    for (std::size_t imageIndex = 0; imageIndex < RenderTextureGlobal::imagesToReleaseAtExit.size(); ++imageIndex)
    {
        vkDestroyImage( GfxDeviceGlobal::device, RenderTextureGlobal::imagesToReleaseAtExit[ imageIndex ], nullptr );
    }

    for (std::size_t imageViewIndex = 0; imageViewIndex < RenderTextureGlobal::imageViewsToReleaseAtExit.size(); ++imageViewIndex)
    {
        vkDestroyImageView( GfxDeviceGlobal::device, RenderTextureGlobal::imageViewsToReleaseAtExit[ imageViewIndex ], nullptr );
    }

    for (std::size_t memoryIndex = 0; memoryIndex < RenderTextureGlobal::memoryToReleaseAtExit.size(); ++memoryIndex)
    {
        vkFreeMemory( GfxDeviceGlobal::device, RenderTextureGlobal::memoryToReleaseAtExit[ memoryIndex ], nullptr );
    }

    for (std::size_t fbIndex = 0; fbIndex < RenderTextureGlobal::fbsToReleaseAtExit.size(); ++fbIndex)
    {
        vkDestroyFramebuffer( GfxDeviceGlobal::device, RenderTextureGlobal::fbsToReleaseAtExit[ fbIndex ], nullptr );
    }

    for (std::size_t rpIndex = 0; rpIndex < RenderTextureGlobal::renderPassesToReleaseAtExit.size(); ++rpIndex)
    {
        vkDestroyRenderPass( GfxDeviceGlobal::device, RenderTextureGlobal::renderPassesToReleaseAtExit[ rpIndex ], nullptr );
    }
}

static void CreateSampler( ae3d::TextureFilter filter, ae3d::TextureWrap wrap, VkSampler& outSampler, int mipLevelCount )
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = filter == ae3d::TextureFilter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerInfo.minFilter = samplerInfo.magFilter;
    samplerInfo.mipmapMode = filter == ae3d::TextureFilter::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = wrap == ae3d::TextureWrap::Repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.mipLodBias = 0;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = (float)mipLevelCount;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    VkResult err = vkCreateSampler( GfxDeviceGlobal::device, &samplerInfo, nullptr, &outSampler );
    AE3D_CHECK_VULKAN( err, "vkCreateSampler" );
    RenderTextureGlobal::samplersToReleaseAtExit.push_back( outSampler );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)outSampler, VK_OBJECT_TYPE_SAMPLER, "sampler" );
}

void ae3d::RenderTexture::Create2D( int aWidth, int aHeight, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter, const char* debugName )
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
    handle = 1;

    // Color

    if (dataType == DataType::UByte)
    {
        colorFormat = GfxDeviceGlobal::colorFormat;
    }
    else if (dataType == DataType::Float)
    {
        colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    else if (dataType == DataType::R32G32)
    {
        colorFormat = VK_FORMAT_R32G32_SFLOAT;
    }
    else
    {
        System::Print( "Unhandled format in 2d render texture\n" );
        colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    }

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
    colorImage.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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
    RenderTextureGlobal::imagesToReleaseAtExit.push_back( color.image );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)color.image, VK_OBJECT_TYPE_IMAGE, debugName );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, color.image, &memReqs );

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize = memReqs.size;

    memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &color.mem );
    AE3D_CHECK_VULKAN( err, "render texture 2d color image memory" );
    RenderTextureGlobal::memoryToReleaseAtExit.push_back( color.mem );
    Statistics::IncAllocCalls();
    Statistics::IncTotalAllocCalls();

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
    RenderTextureGlobal::imageViewsToReleaseAtExit.push_back( color.view );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)color.view, VK_OBJECT_TYPE_IMAGE_VIEW, "render texture 2d color view" );

    // Depth/Stencil

    const VkFormat depthFormat = GfxDeviceGlobal::depthFormat;
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
    RenderTextureGlobal::imagesToReleaseAtExit.push_back( depth.image );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)depth.image, VK_OBJECT_TYPE_IMAGE, "render texture 2d depth" );

    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, depth.image, &memReqs );
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &depth.mem );
    AE3D_CHECK_VULKAN( err, "render texture 2d depth memory" );
    RenderTextureGlobal::memoryToReleaseAtExit.push_back( depth.mem );
    Statistics::IncAllocCalls();
    Statistics::IncTotalAllocCalls();

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
    RenderTextureGlobal::imageViewsToReleaseAtExit.push_back( depth.view );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)depth.view, VK_OBJECT_TYPE_IMAGE_VIEW, "render texture 2d depth view" );

    FlushSetupCommandBuffer();
    CreateRenderPass();

    VkImageView attachments[ 2 ];
    attachments[ 0 ] = color.view;
    attachments[ 1 ] = depth.view;

    VkFramebufferCreateInfo fbufCreateInfo = {};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.renderPass = renderPass;
    fbufCreateInfo.attachmentCount = 2;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = width;
    fbufCreateInfo.height = height;
    fbufCreateInfo.layers = 1;

    err = vkCreateFramebuffer( GfxDeviceGlobal::device, &fbufCreateInfo, nullptr, &frameBuffer );
    AE3D_CHECK_VULKAN( err, "rendertexture framebuffer" );
    RenderTextureGlobal::fbsToReleaseAtExit.push_back( frameBuffer );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)frameBuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "render texture 2d framebuffer" );

    CreateSampler( filter, wrap, sampler, 1 );
}

void ae3d::RenderTexture::CreateCube( int aDimension, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter, const char* debugName )
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
    handle = 1;

    if (dataType == DataType::UByte)
    {
        colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else if (dataType == DataType::Float)
    {
        colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    else if (dataType == DataType::R32G32)
    {
        colorFormat = VK_FORMAT_R32G32_SFLOAT;
    }
    else
    {
        System::Print( "Unhandled format in cube render texture\n" );
        colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    }

    VkImageCreateInfo colorImage = {};
    colorImage.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    colorImage.imageType = VK_IMAGE_TYPE_2D;
    colorImage.format = colorFormat;
    colorImage.extent.width = width;
    colorImage.extent.height = height;
    colorImage.extent.depth = 1;
    colorImage.mipLevels = 1;
    colorImage.arrayLayers = 6;
    colorImage.samples = VK_SAMPLE_COUNT_1_BIT;
    colorImage.tiling = VK_IMAGE_TILING_OPTIMAL;
    colorImage.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    colorImage.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    colorImage.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkResult err = vkCreateImage( GfxDeviceGlobal::device, &colorImage, nullptr, &color.image );
    AE3D_CHECK_VULKAN( err, "vkCreateImage in RenderTextureCube" );
    RenderTextureGlobal::imagesToReleaseAtExit.push_back( color.image );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)color.image, VK_OBJECT_TYPE_IMAGE, debugName );

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, color.image, &memReqs );

    VkMemoryAllocateInfo memAllocInfo = {};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAllocInfo, nullptr, &color.mem );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory in RenderTextureCube" );
    RenderTextureGlobal::memoryToReleaseAtExit.push_back( color.mem );

    err = vkBindImageMemory( GfxDeviceGlobal::device, color.image, color.mem, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindImageMemory in RenderTextureCube" );

    AllocateSetupCommandBuffer();

    SetImageLayout( GfxDeviceGlobal::setupCmdBuffer,
        color.image,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 0, 1 );

    VkImageViewCreateInfo colorImageView = {};
    colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    colorImageView.format = colorFormat;
    colorImageView.flags = 0;
    colorImageView.subresourceRange = {};
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 6;

    colorImageView.image = color.image;
    err = vkCreateImageView( GfxDeviceGlobal::device, &colorImageView, nullptr, &color.view );
    AE3D_CHECK_VULKAN( err, "render texture cube color image view" );
    RenderTextureGlobal::imageViewsToReleaseAtExit.push_back( color.view );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)color.view, VK_OBJECT_TYPE_IMAGE_VIEW, "render texture cube color view" );

    // Depth/Stencil

    const VkFormat depthFormat = GfxDeviceGlobal::depthFormat;
    VkImageCreateInfo depthImage = colorImage;
    depthImage.format = depthFormat;
    depthImage.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageViewCreateInfo depthStencilView = {};
    depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    depthStencilView.format = depthFormat;
    depthStencilView.flags = 0;
    depthStencilView.subresourceRange = {};
    depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthStencilView.subresourceRange.baseMipLevel = 0;
    depthStencilView.subresourceRange.levelCount = 1;
    depthStencilView.subresourceRange.baseArrayLayer = 0;
    depthStencilView.subresourceRange.layerCount = 6;

    err = vkCreateImage( GfxDeviceGlobal::device, &depthImage, nullptr, &depth.image );
    AE3D_CHECK_VULKAN( err, "render texture cube depth image" );
    RenderTextureGlobal::imagesToReleaseAtExit.push_back( depth.image );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)depth.image, VK_OBJECT_TYPE_IMAGE, "render texture cube depth" );

    vkGetImageMemoryRequirements( GfxDeviceGlobal::device, depth.image, &memReqs );

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.allocationSize = memReqs.size;
    
    memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &depth.mem );
    AE3D_CHECK_VULKAN( err, "render texture cube depth memory" );
    RenderTextureGlobal::memoryToReleaseAtExit.push_back( depth.mem );
    Statistics::IncAllocCalls();
    Statistics::IncTotalAllocCalls();

    err = vkBindImageMemory( GfxDeviceGlobal::device, depth.image, depth.mem, 0 );
    AE3D_CHECK_VULKAN( err, "render texture cube depth bind" );

    SetImageLayout( GfxDeviceGlobal::setupCmdBuffer,
        depth.image,
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 0, 1 );

    depthStencilView.image = depth.image;
    err = vkCreateImageView( GfxDeviceGlobal::device, &depthStencilView, nullptr, &depth.view );
    AE3D_CHECK_VULKAN( err, "render texture cube depth view" );
    RenderTextureGlobal::imageViewsToReleaseAtExit.push_back( depth.view );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)depth.view, VK_OBJECT_TYPE_IMAGE_VIEW, "render texture cube depth view" );

    FlushSetupCommandBuffer();
    CreateRenderPass();

    VkImageView attachments[ 2 ];
    attachments[ 0 ] = color.view;
    attachments[ 1 ] = depth.view;

    VkFramebufferCreateInfo fbufCreateInfo = {};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.renderPass = renderPass;
    fbufCreateInfo.attachmentCount = 2;
    fbufCreateInfo.pAttachments = attachments;
    fbufCreateInfo.width = width;
    fbufCreateInfo.height = height;
    fbufCreateInfo.layers = 1;

    err = vkCreateFramebuffer( GfxDeviceGlobal::device, &fbufCreateInfo, nullptr, &frameBuffer );
    AE3D_CHECK_VULKAN( err, "rendertexture framebuffer" );
    RenderTextureGlobal::fbsToReleaseAtExit.push_back( frameBuffer );
    debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)frameBuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "render texture cube framebuffer" );

    CreateSampler( filter, wrap, sampler, mipLevelCount );
}

void ae3d::RenderTexture::CreateRenderPass()
{
    VkAttachmentDescription attachments[ 2 ];
    attachments[ 0 ].format = colorFormat;
    attachments[ 0 ].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[ 0 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 0 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[ 0 ].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[ 0 ].flags = 0;

    attachments[ 1 ].format = GfxDeviceGlobal::depthFormat;
    attachments[ 1 ].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[ 1 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 1 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[ 1 ].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[ 1 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[ 1 ].flags = 0;

    VkAttachmentReference colorReference = {};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference = {};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorReference;
    subpass.pResolveAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depthReference;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;

    VkResult err = vkCreateRenderPass( GfxDeviceGlobal::device, &renderPassInfo, nullptr, &renderPass );
    AE3D_CHECK_VULKAN( err, "RenderTexture vkCreateRenderPass" );

    RenderTextureGlobal::renderPassesToReleaseAtExit.push_back( renderPass );
}
