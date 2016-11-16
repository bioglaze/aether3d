#include "GfxDevice.hpp"
#include <cstdint>
#include <chrono>
#include <map>
#include <vector> 
#include <string>
#include <sstream>
#include <vulkan/vulkan.h>
#include "Macros.hpp"
#include "RenderTexture.hpp"
#include "System.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "VulkanUtils.hpp"
#if VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#endif
#if VK_USE_PLATFORM_XCB_KHR
#include <X11/Xlib-xcb.h>
#endif

#define AE3D_DESCRIPTOR_SETS_COUNT 250

// Current implementation loosely based on samples by Sascha Willems - https://github.com/SaschaWillems/Vulkan, licensed under MIT license

PFN_vkCreateSwapchainKHR createSwapchainKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR getPhysicalDeviceSurfacePresentModesKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR getPhysicalDeviceSurfaceSupportKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR getPhysicalDeviceSurfaceFormatsKHR = nullptr;
PFN_vkGetSwapchainImagesKHR getSwapchainImagesKHR = nullptr;
PFN_vkAcquireNextImageKHR acquireNextImageKHR = nullptr;
PFN_vkQueuePresentKHR queuePresentKHR = nullptr;

struct Ubo
{
    VkBuffer ubo = VK_NULL_HANDLE;
    VkDeviceMemory uboMemory;
    VkDescriptorBufferInfo uboDesc;
    std::uint8_t* uboData = nullptr;
};

namespace Stats
{
    int drawCalls = 0;
    int barrierCalls = 0;
    int fenceCalls = 0;
    float frameTimeMS = 0;
    std::chrono::time_point< std::chrono::high_resolution_clock > startFrameTimePoint;
}

namespace GfxDeviceGlobal
{
    struct SwapchainBuffer
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    };

    struct DepthStencil
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory mem = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
    } depthStencil;
    
    struct MsaaTarget
    {
        VkImage colorImage = VK_NULL_HANDLE;
        VkImageView colorView = VK_NULL_HANDLE;
        VkDeviceMemory colorMem = VK_NULL_HANDLE;

        VkImage depthImage = VK_NULL_HANDLE;
        VkImageView depthView = VK_NULL_HANDLE;
        VkDeviceMemory depthMem = VK_NULL_HANDLE;
    } msaaTarget;

    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkClearColorValue clearColor;
    
    std::vector< VkCommandBuffer > drawCmdBuffers;
    VkCommandBuffer setupCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer prePresentCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer postPresentCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer computeCmdBuffer = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    VkFormat colorFormat;
    VkFormat depthFormat;
    VkColorSpaceKHR colorSpace;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;
    std::vector< VkImage > swapchainImages;
    std::vector< SwapchainBuffer > swapchainBuffers;
    std::vector< VkFramebuffer > frameBuffers;
    VkSemaphore presentCompleteSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::map< unsigned, VkPipeline > psoCache;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector< VkDescriptorSet > descriptorSets;
    int descriptorSetIndex = 0;
    std::uint32_t queueNodeIndex = UINT32_MAX;
    std::uint32_t currentBuffer = 0;
    ae3d::RenderTexture* renderTexture0 = nullptr;
    VkFramebuffer frameBuffer0 = VK_NULL_HANDLE;
    VkImageView view0 = VK_NULL_HANDLE;
    VkSampler sampler0 = VK_NULL_HANDLE;
    std::vector< VkBuffer > pendingFreeVBs;
    std::vector< Ubo > frameUbos;
    VkSampleCountFlagBits msaaSampleBits = VK_SAMPLE_COUNT_1_BIT;
}

namespace ae3d
{
    namespace System
    {
        namespace Statistics
        {
            std::string GetStatistics()
            {
                std::stringstream stm;
                stm << "frame time: " << Stats::frameTimeMS << " ms\n";
                stm << "draw calls: " << Stats::drawCalls << "\n";
                stm << "barrier calls: " << Stats::barrierCalls << "\n";
                stm << "fence calls: " << Stats::fenceCalls << "\n";

                return stm.str();
            }
        }
    }
}

namespace WindowGlobal
{
#if VK_USE_PLATFORM_WIN32_KHR
    extern HWND hwnd;
#endif
#if VK_USE_PLATFORM_XCB_KHR
    extern xcb_connection_t* connection;
    extern xcb_window_t window;
#endif
    extern int windowWidth;
    extern int windowHeight;
}

namespace ae3d
{
    void GetMemoryType( std::uint32_t typeBits, VkFlags properties, std::uint32_t* typeIndex )
    {
        for (std::uint32_t i = 0; i < 32; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((GfxDeviceGlobal::deviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties) == properties)
                {
                    *typeIndex = i;
                    return;
                }
            }
            typeBits >>= 1;
        }

        ae3d::System::Assert( false, "could not get memory type" );
    }

    void CreateMsaaColor()
    {
        //assert( (deviceProperties.limits.framebufferColorSampleCounts >= SampleCount) && (deviceProperties.limits.framebufferDepthSampleCounts >= SAMPLE_COUNT) );

        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = GfxDeviceGlobal::colorFormat;
        info.extent.width = WindowGlobal::windowWidth;
        info.extent.height = WindowGlobal::windowHeight;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.samples = GfxDeviceGlobal::msaaSampleBits;
        info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult err = vkCreateImage( GfxDeviceGlobal::device, &info, nullptr, &GfxDeviceGlobal::msaaTarget.colorImage );
        AE3D_CHECK_VULKAN( err, "Create MSAA color" );

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.colorImage, &memReqs );
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex );
        
        err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &GfxDeviceGlobal::msaaTarget.colorMem );
        AE3D_CHECK_VULKAN( err, "Create MSAA color" );

        vkBindImageMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.colorImage, GfxDeviceGlobal::msaaTarget.colorMem, 0 );

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = GfxDeviceGlobal::msaaTarget.colorImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = GfxDeviceGlobal::colorFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        err = vkCreateImageView( GfxDeviceGlobal::device, &viewInfo, nullptr, &GfxDeviceGlobal::msaaTarget.colorView );
        AE3D_CHECK_VULKAN( err, "Create MSAA view" );
    }

    void CreateMsaaDepth()
    {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = GfxDeviceGlobal::depthFormat;
        info.extent.width = WindowGlobal::windowWidth;
        info.extent.height = WindowGlobal::windowHeight;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.samples = GfxDeviceGlobal::msaaSampleBits;
        info.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult err = vkCreateImage( GfxDeviceGlobal::device, &info, nullptr, &GfxDeviceGlobal::msaaTarget.depthImage );
        AE3D_CHECK_VULKAN( err, "MSAA depth image" );

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.depthImage, &memReqs );
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex );

        err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &GfxDeviceGlobal::msaaTarget.depthMem );
        AE3D_CHECK_VULKAN( err, "MSAA depth image" );
        vkBindImageMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.depthImage, GfxDeviceGlobal::msaaTarget.depthMem, 0 );

        // Create image view for the MSAA target
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = GfxDeviceGlobal::msaaTarget.depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = GfxDeviceGlobal::depthFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;

        err = vkCreateImageView( GfxDeviceGlobal::device, &viewInfo, nullptr, &GfxDeviceGlobal::msaaTarget.depthView );
        AE3D_CHECK_VULKAN( err, "MSAA depth image" );
    }

    void CreatePSO( VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc,
                    ae3d::GfxDevice::CullMode cullMode, unsigned hash )
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
        inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterizationState = {};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        
        if (cullMode == ae3d::GfxDevice::CullMode::Off)
        {
            rasterizationState.cullMode = VK_CULL_MODE_NONE;
        }
        else if (cullMode == ae3d::GfxDevice::CullMode::Back)
        {
            rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
        }
        else if (cullMode == ae3d::GfxDevice::CullMode::Front)
        {
            rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
        }
        else
        {
            ae3d::System::Assert( false, "unhandled cull mode" );
        }

        rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationState.depthClampEnable = VK_FALSE;
        rasterizationState.rasterizerDiscardEnable = VK_FALSE;
        rasterizationState.depthBiasEnable = VK_FALSE;
        rasterizationState.lineWidth = 1;

        VkPipelineColorBlendStateCreateInfo colorBlendState = {};
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        VkPipelineColorBlendAttachmentState blendAttachmentState[ 1 ] = {};
        blendAttachmentState[ 0 ].colorWriteMask = 0xF;
        blendAttachmentState[ 0 ].blendEnable = blendMode != ae3d::GfxDevice::BlendMode::Off ? VK_TRUE : VK_FALSE;
        if (blendMode == ae3d::GfxDevice::BlendMode::AlphaBlend)
        {
            blendAttachmentState[ 0 ].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blendAttachmentState[ 0 ].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttachmentState[ 0 ].colorBlendOp = VK_BLEND_OP_ADD;
            blendAttachmentState[ 0 ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachmentState[ 0 ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            blendAttachmentState[ 0 ].alphaBlendOp = VK_BLEND_OP_ADD;
        }
        else if (blendMode == ae3d::GfxDevice::BlendMode::Additive)
        {
            blendAttachmentState[ 0 ].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachmentState[ 0 ].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachmentState[ 0 ].colorBlendOp = VK_BLEND_OP_ADD;
            blendAttachmentState[ 0 ].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachmentState[ 0 ].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachmentState[ 0 ].alphaBlendOp = VK_BLEND_OP_ADD;
        }
        colorBlendState.attachmentCount = 1;
        colorBlendState.pAttachments = blendAttachmentState;

        VkPipelineDynamicStateCreateInfo dynamicState = {};
        VkDynamicState dynamicStateEnables[ 2 ];
        dynamicStateEnables[ 0 ] = VK_DYNAMIC_STATE_VIEWPORT;
        dynamicStateEnables[ 1 ] = VK_DYNAMIC_STATE_SCISSOR;
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pDynamicStates = &dynamicStateEnables[ 0 ];
        dynamicState.dynamicStateCount = 2;

        VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
        depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilState.depthTestEnable = depthFunc == ae3d::GfxDevice::DepthFunc::NoneWriteOff ? VK_FALSE : VK_TRUE;
        depthStencilState.depthWriteEnable = depthFunc == ae3d::GfxDevice::DepthFunc::LessOrEqualWriteOn ? VK_TRUE : VK_FALSE;
        if (depthFunc == ae3d::GfxDevice::DepthFunc::LessOrEqualWriteOn || depthFunc == ae3d::GfxDevice::DepthFunc::LessOrEqualWriteOff)
        {
            depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        }
        else if (depthFunc == ae3d::GfxDevice::DepthFunc::NoneWriteOff)
        {
            depthStencilState.depthCompareOp = VK_COMPARE_OP_NEVER;
        }
        else
        {
            ae3d::System::Assert( false, "unhandled depth function" );
        }
        depthStencilState.depthBoundsTestEnable = VK_FALSE;
        depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
        depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
        depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
        depthStencilState.stencilTestEnable = VK_FALSE;
        depthStencilState.front = depthStencilState.back;

        VkPipelineMultisampleStateCreateInfo multisampleState = {};
        multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleState.pSampleMask = nullptr;
        multisampleState.rasterizationSamples = GfxDeviceGlobal::msaaSampleBits;

        VkPipelineShaderStageCreateInfo shaderStages[ 2 ] = { {},{} };

        shaderStages[ 0 ] = shader.GetVertexInfo();
        shaderStages[ 1 ] = shader.GetFragmentInfo();

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};

        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = GfxDeviceGlobal::pipelineLayout;
        pipelineCreateInfo.renderPass = GfxDeviceGlobal::renderPass;
        pipelineCreateInfo.pVertexInputState = vertexBuffer.GetInputState();
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.stageCount = 2;
        pipelineCreateInfo.pStages = shaderStages;
        pipelineCreateInfo.pDynamicState = &dynamicState;

        VkPipeline pso;

        VkResult err = vkCreateGraphicsPipelines( GfxDeviceGlobal::device, GfxDeviceGlobal::pipelineCache, 1, &pipelineCreateInfo,
                                                  nullptr, &pso );
        AE3D_CHECK_VULKAN( err, "vkCreateGraphicsPipelines" );

        GfxDeviceGlobal::psoCache[ hash ] = pso;
    }

    void AllocateCommandBuffers()
    {
        System::Assert( GfxDeviceGlobal::cmdPool != VK_NULL_HANDLE, "command pool not initialized" );
        System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );
        System::Assert( GfxDeviceGlobal::swapchainBuffers.size() > 0, "imageCount is 0" );

        GfxDeviceGlobal::drawCmdBuffers.resize( GfxDeviceGlobal::swapchainBuffers.size() );

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = GfxDeviceGlobal::cmdPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = (std::uint32_t)GfxDeviceGlobal::drawCmdBuffers.size();

        VkResult err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, GfxDeviceGlobal::drawCmdBuffers.data() );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );

        commandBufferAllocateInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::postPresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::prePresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );

        VkCommandBufferAllocateInfo computeBufAllocateInfo = {};
        computeBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        computeBufAllocateInfo.commandPool = GfxDeviceGlobal::cmdPool;
        computeBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        computeBufAllocateInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &computeBufAllocateInfo, &GfxDeviceGlobal::computeCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
    }

    void SubmitPrePresentBarrier()
    {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::prePresentCmdBuffer, &cmdBufInfo );
        AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

        VkImageMemoryBarrier prePresentBarrier = {};
        prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        prePresentBarrier.pNext = nullptr;
        prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        prePresentBarrier.image = GfxDeviceGlobal::swapchainBuffers[ GfxDeviceGlobal::currentBuffer ].image;

        vkCmdPipelineBarrier(
            GfxDeviceGlobal::prePresentCmdBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &prePresentBarrier );
        ++Stats::barrierCalls;

        err = vkEndCommandBuffer( GfxDeviceGlobal::prePresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &GfxDeviceGlobal::prePresentCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
        AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );
    }

    void SubmitPostPresentBarrier()
    {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::postPresentCmdBuffer, &cmdBufInfo );
        AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

        VkImageMemoryBarrier postPresentBarrier = {};
        postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postPresentBarrier.pNext = nullptr;
        postPresentBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        postPresentBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        postPresentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        postPresentBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        postPresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postPresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        postPresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        postPresentBarrier.image = GfxDeviceGlobal::swapchainBuffers[ GfxDeviceGlobal::currentBuffer ].image;

        vkCmdPipelineBarrier(
            GfxDeviceGlobal::postPresentCmdBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &postPresentBarrier );
        ++Stats::barrierCalls;

        err = vkEndCommandBuffer( GfxDeviceGlobal::postPresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );

        VkSubmitInfo submitPostInfo = {};
        submitPostInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitPostInfo.commandBufferCount = 1;
        submitPostInfo.pCommandBuffers = &GfxDeviceGlobal::postPresentCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitPostInfo, VK_NULL_HANDLE );
        AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );

        auto tEnd = std::chrono::high_resolution_clock::now();
        auto tDiff = std::chrono::duration<double, std::milli>( tEnd - Stats::startFrameTimePoint ).count();
        Stats::frameTimeMS = static_cast< float >(tDiff);
    }
    
    void AllocateSetupCommandBuffer()
    {
        System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized." );

        if (GfxDeviceGlobal::setupCmdBuffer != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers( GfxDeviceGlobal::device, GfxDeviceGlobal::cmdPool, 1, &GfxDeviceGlobal::setupCmdBuffer );
            GfxDeviceGlobal::setupCmdBuffer = VK_NULL_HANDLE;
        }

        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.queueFamilyIndex = GfxDeviceGlobal::queueNodeIndex;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VkResult err = vkCreateCommandPool( GfxDeviceGlobal::device, &cmdPoolInfo, nullptr, &GfxDeviceGlobal::cmdPool );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );

        VkCommandBufferAllocateInfo info = {};
        info.commandBufferCount = 1;
        info.commandPool = GfxDeviceGlobal::cmdPool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.pNext = nullptr;
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &info, &GfxDeviceGlobal::setupCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );

        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        err = vkBeginCommandBuffer( GfxDeviceGlobal::setupCmdBuffer, &cmdBufInfo );
        AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );
    }

    void InitSwapChain()
    {
        System::Assert( GfxDeviceGlobal::instance != VK_NULL_HANDLE, "instance not initialized." );
        System::Assert( GfxDeviceGlobal::physicalDevice != VK_NULL_HANDLE, "physicalDevice not initialized." );

        VkResult err;
#if VK_USE_PLATFORM_WIN32_KHR
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.hinstance = GetModuleHandle( nullptr );
        surfaceCreateInfo.hwnd = WindowGlobal::hwnd;
        err = vkCreateWin32SurfaceKHR( GfxDeviceGlobal::instance, &surfaceCreateInfo, nullptr, &GfxDeviceGlobal::surface );
#endif
#if VK_USE_PLATFORM_XCB_KHR
        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.connection = WindowGlobal::connection;
        surfaceCreateInfo.window = WindowGlobal::window;
        err = vkCreateXcbSurfaceKHR( GfxDeviceGlobal::instance, &surfaceCreateInfo, nullptr, &GfxDeviceGlobal::surface );
#endif
        AE3D_CHECK_VULKAN( err, "create surface" );
        System::Assert( GfxDeviceGlobal::surface != VK_NULL_HANDLE, "no surface" );
        
        std::uint32_t queueCount;
        vkGetPhysicalDeviceQueueFamilyProperties( GfxDeviceGlobal::physicalDevice, &queueCount, nullptr );

        std::vector < VkQueueFamilyProperties > queueProps( queueCount );
        vkGetPhysicalDeviceQueueFamilyProperties( GfxDeviceGlobal::physicalDevice, &queueCount, queueProps.data() );
        System::Assert( queueCount >= 1, "no queues" );

        std::vector< VkBool32 > supportsPresent( queueCount );

        for (std::uint32_t i = 0; i < queueCount; ++i)
        {
            getPhysicalDeviceSurfaceSupportKHR( GfxDeviceGlobal::physicalDevice, i,
                GfxDeviceGlobal::surface,
                &supportsPresent[ i ] );
        }

        std::uint32_t graphicsQueueNodeIndex = UINT32_MAX;
        std::uint32_t presentQueueNodeIndex = UINT32_MAX;
        std::uint32_t computeQueueNodeIndex = UINT32_MAX;

        for (std::uint32_t i = 0; i < queueCount; ++i)
        {
            if ((queueProps[ i ].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 && computeQueueNodeIndex == UINT32_MAX)
            {
                computeQueueNodeIndex = i;
            }
            
            if ((queueProps[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
            {
                if (graphicsQueueNodeIndex == UINT32_MAX)
                {
                    graphicsQueueNodeIndex = i;
                }

                if (supportsPresent[ i ] == VK_TRUE)
                {
                    graphicsQueueNodeIndex = i;
                    presentQueueNodeIndex = i;
                    break;
                }
            }
        }

        if (presentQueueNodeIndex == UINT32_MAX)
        {
            // If there's no queue that supports both present and graphics
            // try to find a separate present queue
            for (std::uint32_t q = 0; q < queueCount; ++q)
            {
                if (supportsPresent[ q ] == VK_TRUE)
                {
                    presentQueueNodeIndex = q;
                    break;
                }
            }
        }

        if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
        {
            System::Assert( false, "graphics or present queue not found" );
        }

        if (graphicsQueueNodeIndex != presentQueueNodeIndex)
        {
            System::Assert( false, "graphics and present queues must have the same index" );
        }

        if (computeQueueNodeIndex == UINT32_MAX)
        {
            System::Assert( false, "compute queue not found" );
        }

        vkGetDeviceQueue( GfxDeviceGlobal::device, computeQueueNodeIndex, 0, &GfxDeviceGlobal::computeQueue );

        GfxDeviceGlobal::queueNodeIndex = graphicsQueueNodeIndex;

        std::uint32_t formatCount;
        err = getPhysicalDeviceSurfaceFormatsKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &formatCount, nullptr );
        AE3D_CHECK_VULKAN( err, "getPhysicalDeviceSurfaceFormatsKHR" );
        ae3d::System::Assert( formatCount > 0, "no formats" );
        
        std::vector< VkSurfaceFormatKHR > surfFormats( formatCount );
        err = getPhysicalDeviceSurfaceFormatsKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &formatCount, surfFormats.data() );
        AE3D_CHECK_VULKAN( err, "getPhysicalDeviceSurfaceFormatsKHR" );

        bool foundSRGB = false;
        VkFormat sRGBFormat = VK_FORMAT_B8G8R8A8_SRGB;

        for (std::size_t formatIndex = 0; formatIndex < surfFormats.size(); ++formatIndex)
        {
            if (surfFormats[ formatIndex ].format == VK_FORMAT_B8G8R8A8_SRGB || surfFormats[ formatIndex ].format == VK_FORMAT_R8G8B8A8_SRGB)
            {
                sRGBFormat = surfFormats[ formatIndex ].format;
                foundSRGB = true;
            }
        }

        if (foundSRGB)
        {
            GfxDeviceGlobal::colorFormat = sRGBFormat;
        }
        else if (formatCount == 1 && surfFormats[ 0 ].format == VK_FORMAT_UNDEFINED)
        {
            GfxDeviceGlobal::colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
        }
        else
        {
            System::Assert( formatCount >= 1, "no formats" );
            GfxDeviceGlobal::colorFormat = surfFormats[ 0 ].format;
        }

        GfxDeviceGlobal::colorSpace = surfFormats[ 0 ].colorSpace;
    }

    void SetupSwapChain()
    {
        System::Assert( GfxDeviceGlobal::surface != VK_NULL_HANDLE, "surface not created yet" );

        VkSurfaceCapabilitiesKHR surfCaps;
        VkResult err = getPhysicalDeviceSurfaceCapabilitiesKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &surfCaps );
        AE3D_CHECK_VULKAN( err, "getPhysicalDeviceSurfaceCapabilitiesKHR" );

        std::uint32_t presentModeCount = 0;
        err = getPhysicalDeviceSurfacePresentModesKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &presentModeCount, nullptr );
        AE3D_CHECK_VULKAN( err, "getPhysicalDeviceSurfacePresentModesKHR" );
        ae3d::System::Assert( presentModeCount > 0, "no present modes" );
        
        std::vector< VkPresentModeKHR > presentModes( presentModeCount );
        err = getPhysicalDeviceSurfacePresentModesKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &presentModeCount, presentModes.data() );
        AE3D_CHECK_VULKAN( err, "getPhysicalDeviceSurfacePresentModesKHR" );

        VkExtent2D swapchainExtent = {};
        if (surfCaps.currentExtent.width == 0)
        {
            swapchainExtent.width = WindowGlobal::windowWidth;
            swapchainExtent.height = WindowGlobal::windowHeight;
        }
        else
        {
            swapchainExtent = surfCaps.currentExtent;
            WindowGlobal::windowWidth = surfCaps.currentExtent.width;
            WindowGlobal::windowHeight = surfCaps.currentExtent.height;
        }

        std::uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;

        if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
        {
            desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
        }

        VkSurfaceTransformFlagsKHR preTransform;

        if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        {
            preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        }
        else
        {
            preTransform = surfCaps.currentTransform;
        }

        VkSwapchainCreateInfoKHR swapchainInfo = {};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.pNext = nullptr;
        swapchainInfo.surface = GfxDeviceGlobal::surface;
        swapchainInfo.minImageCount = desiredNumberOfSwapchainImages;
        swapchainInfo.imageFormat = GfxDeviceGlobal::colorFormat;
        swapchainInfo.imageColorSpace = GfxDeviceGlobal::colorSpace;
        swapchainInfo.imageExtent = { swapchainExtent.width, swapchainExtent.height };
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.pQueueFamilyIndices = nullptr;
        swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        err = createSwapchainKHR( GfxDeviceGlobal::device, &swapchainInfo, nullptr, &GfxDeviceGlobal::swapChain );
        AE3D_CHECK_VULKAN( err, "swapchain" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::swapChain, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, "swap chain" );

        std::uint32_t imageCount;
        err = getSwapchainImagesKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, &imageCount, nullptr );
        AE3D_CHECK_VULKAN( err, "getSwapchainImagesKHR" );
        ae3d::System::Assert( imageCount > 0, "imageCount" );
        
        GfxDeviceGlobal::swapchainImages.resize( imageCount );

        err = getSwapchainImagesKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, &imageCount, GfxDeviceGlobal::swapchainImages.data() );
        AE3D_CHECK_VULKAN( err, "getSwapchainImagesKHR" );

        GfxDeviceGlobal::swapchainBuffers.resize( imageCount );

        for (std::uint32_t i = 0; i < imageCount; ++i)
        {
            VkImageViewCreateInfo colorAttachmentView = {};
            colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            colorAttachmentView.pNext = nullptr;
            colorAttachmentView.format = GfxDeviceGlobal::colorFormat;
            colorAttachmentView.components = {
                VK_COMPONENT_SWIZZLE_R,
                VK_COMPONENT_SWIZZLE_G,
                VK_COMPONENT_SWIZZLE_B,
                VK_COMPONENT_SWIZZLE_A
            };
            colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorAttachmentView.subresourceRange.baseMipLevel = 0;
            colorAttachmentView.subresourceRange.levelCount = 1;
            colorAttachmentView.subresourceRange.baseArrayLayer = 0;
            colorAttachmentView.subresourceRange.layerCount = 1;
            colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
            colorAttachmentView.flags = 0;

            GfxDeviceGlobal::swapchainBuffers[ i ].image = GfxDeviceGlobal::swapchainImages[ i ];

            SetImageLayout(
            GfxDeviceGlobal::setupCmdBuffer,
            GfxDeviceGlobal::swapchainBuffers[ i ].image,
            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, 0, 1 );
            
            colorAttachmentView.image = GfxDeviceGlobal::swapchainBuffers[ i ].image;

            err = vkCreateImageView( GfxDeviceGlobal::device, &colorAttachmentView, nullptr, &GfxDeviceGlobal::swapchainBuffers[ i ].view );
            AE3D_CHECK_VULKAN( err, "vkCreateImageView" );
        }
    }

    void LoadFunctionPointers()
    {
        System::Assert( GfxDeviceGlobal::device != nullptr, "Device not initialized yet" );
        System::Assert( GfxDeviceGlobal::instance != nullptr, "Instance not initialized yet" );

        createSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr( GfxDeviceGlobal::device, "vkCreateSwapchainKHR" );
        System::Assert( createSwapchainKHR != nullptr, "Could not load vkCreateSwapchainKHR function" );

        getPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr( GfxDeviceGlobal::instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR" );
        System::Assert( getPhysicalDeviceSurfaceCapabilitiesKHR != nullptr, "Could not load vkGetPhysicalDeviceSurfaceCapabilitiesKHR function" );

        getPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr( GfxDeviceGlobal::instance, "vkGetPhysicalDeviceSurfacePresentModesKHR" );
        System::Assert( getPhysicalDeviceSurfacePresentModesKHR != nullptr, "Could not load vkGetPhysicalDeviceSurfacePresentModesKHR function" );

        getPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr( GfxDeviceGlobal::instance, "vkGetPhysicalDeviceSurfaceFormatsKHR" );
        System::Assert( getPhysicalDeviceSurfaceFormatsKHR != nullptr, "Could not load vkGetPhysicalDeviceSurfaceFormatsKHR function" );

        getPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr( GfxDeviceGlobal::instance, "vkGetPhysicalDeviceSurfaceSupportKHR" );
        System::Assert( getPhysicalDeviceSurfaceSupportKHR != nullptr, "Could not load vkGetPhysicalDeviceSurfaceSupportKHR function" );

        getSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr( GfxDeviceGlobal::device, "vkGetSwapchainImagesKHR" );
        System::Assert( getSwapchainImagesKHR != nullptr, "Could not load vkGetSwapchainImagesKHR function" );

        acquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr( GfxDeviceGlobal::device, "vkAcquireNextImageKHR" );
        System::Assert( acquireNextImageKHR != nullptr, "Could not load vkAcquireNextImageKHR function" );

        queuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr( GfxDeviceGlobal::device, "vkQueuePresentKHR" );
        System::Assert( queuePresentKHR != nullptr, "Could not load vkQueuePresentKHR function" );
    }

    void CreateDevice()
    {
        System::Assert( GfxDeviceGlobal::instance != VK_NULL_HANDLE, "instance not created yet" );

        std::uint32_t gpuCount;
        VkResult result = vkEnumeratePhysicalDevices( GfxDeviceGlobal::instance, &gpuCount, nullptr );
        AE3D_CHECK_VULKAN( result, "vkEnumeratePhysicalDevices" );
        if (gpuCount < 1)
        {
            System::Print( "Your system doesn't have Vulkan capable GPU.\n" );
        }

        result = vkEnumeratePhysicalDevices( GfxDeviceGlobal::instance, &gpuCount, &GfxDeviceGlobal::physicalDevice );
        AE3D_CHECK_VULKAN( result, "vkEnumeratePhysicalDevices" );

        // Finds graphics queue.
        std::uint32_t queueCount;
        vkGetPhysicalDeviceQueueFamilyProperties( GfxDeviceGlobal::physicalDevice, &queueCount, nullptr );
        if (queueCount < 1)
        {
            System::Print( "Your system doesn't have physical Vulkan devices.\n" );
        }

        std::vector< VkQueueFamilyProperties > queueProps( queueCount );
        vkGetPhysicalDeviceQueueFamilyProperties( GfxDeviceGlobal::physicalDevice, &queueCount, queueProps.data() );
        std::uint32_t graphicsQueueIndex = 0;

        for (graphicsQueueIndex = 0; graphicsQueueIndex < queueCount; ++graphicsQueueIndex)
        {
            if (queueProps[ graphicsQueueIndex ].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                break;
            }
        }

        System::Assert( graphicsQueueIndex < queueCount, "graphicsQueueIndex" );

        float queuePriorities = 0;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriorities;

        std::vector< const char* > deviceExtensions;
        deviceExtensions.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );

        std::uint32_t deviceExtensionCount;
        vkEnumerateDeviceExtensionProperties( GfxDeviceGlobal::physicalDevice, nullptr, &deviceExtensionCount, nullptr );
        std::vector< VkExtensionProperties > availableDeviceExtensions( deviceExtensionCount );
        vkEnumerateDeviceExtensionProperties( GfxDeviceGlobal::physicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtensions.data() );

        for (auto& i : availableDeviceExtensions)
        {
            if (std::string( i.extensionName ) == std::string( VK_EXT_DEBUG_MARKER_EXTENSION_NAME ))
            {
                deviceExtensions.push_back( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );
                debug::hasMarker = true;
            }
        }

        VkPhysicalDeviceFeatures enabledFeatures = {};
        enabledFeatures.tessellationShader = true;
        enabledFeatures.shaderTessellationAndGeometryPointSize = true;
        enabledFeatures.shaderClipDistance = true;
        enabledFeatures.shaderCullDistance = true;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = nullptr;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast< std::uint32_t >( deviceExtensions.size() );
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        result = vkCreateDevice( GfxDeviceGlobal::physicalDevice, &deviceCreateInfo, nullptr, &GfxDeviceGlobal::device );
        AE3D_CHECK_VULKAN( result, "device" );

        vkGetPhysicalDeviceMemoryProperties( GfxDeviceGlobal::physicalDevice, &GfxDeviceGlobal::deviceMemoryProperties );
        vkGetDeviceQueue( GfxDeviceGlobal::device, graphicsQueueIndex, 0, &GfxDeviceGlobal::graphicsQueue );

        const std::vector< VkFormat > depthFormats = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
        bool depthFormatFound = false;
        for (auto& format : depthFormats)
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties( GfxDeviceGlobal::physicalDevice, format, &formatProps );

            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                GfxDeviceGlobal::depthFormat = format;
                depthFormatFound = true;
                break;
            }
        }

        System::Assert( depthFormatFound, "No suitable depth format found" );
    }

    void CreateFramebufferNonMSAA()
    {
        VkImageView attachments[ 2 ];

        // Depth/Stencil attachment is the same for all frame buffers
        attachments[ 1 ] = GfxDeviceGlobal::depthStencil.view;

        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.pNext = nullptr;
        frameBufferCreateInfo.renderPass = GfxDeviceGlobal::renderPass;
        frameBufferCreateInfo.attachmentCount = 2;
        frameBufferCreateInfo.pAttachments = attachments;
        frameBufferCreateInfo.width = static_cast< std::uint32_t >( WindowGlobal::windowWidth );
        frameBufferCreateInfo.height = static_cast< std::uint32_t >( WindowGlobal::windowHeight );
        frameBufferCreateInfo.layers = 1;

        // Create frame buffers for every swap chain image
        GfxDeviceGlobal::frameBuffers.resize( GfxDeviceGlobal::swapchainBuffers.size() );
        
        for (std::uint32_t i = 0; i < GfxDeviceGlobal::frameBuffers.size(); i++)
        {
            attachments[ 0 ] = GfxDeviceGlobal::swapchainBuffers[ i ].view;
            VkResult err = vkCreateFramebuffer( GfxDeviceGlobal::device, &frameBufferCreateInfo, nullptr, &GfxDeviceGlobal::frameBuffers[ i ] );
            AE3D_CHECK_VULKAN( err, "vkCreateFramebuffer" );
        }
    }

    void CreateFramebufferMSAA()
    {
        System::Assert( GfxDeviceGlobal::msaaTarget.colorImage != VK_NULL_HANDLE, "MSAA image not created" );

        VkImageView attachments[ 4 ] = {};
        attachments[ 0 ] = GfxDeviceGlobal::msaaTarget.colorView;
        // attachment[1] = swapchain image
        attachments[ 2 ] = GfxDeviceGlobal::msaaTarget.depthView;
        attachments[ 3 ] = GfxDeviceGlobal::depthStencil.view;

        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.pNext = nullptr;
        frameBufferCreateInfo.renderPass = GfxDeviceGlobal::renderPass;
        frameBufferCreateInfo.attachmentCount = 4;
        frameBufferCreateInfo.pAttachments = attachments;
        frameBufferCreateInfo.width = static_cast< std::uint32_t >(WindowGlobal::windowWidth);
        frameBufferCreateInfo.height = static_cast< std::uint32_t >(WindowGlobal::windowHeight);
        frameBufferCreateInfo.layers = 1;

        // Create frame buffers for every swap chain image
        GfxDeviceGlobal::frameBuffers.resize( GfxDeviceGlobal::swapchainBuffers.size() );

        for (std::uint32_t i = 0; i < GfxDeviceGlobal::frameBuffers.size(); i++)
        {
            attachments[ 1 ] = GfxDeviceGlobal::swapchainBuffers[ i ].view;
            VkResult err = vkCreateFramebuffer( GfxDeviceGlobal::device, &frameBufferCreateInfo, nullptr, &GfxDeviceGlobal::frameBuffers[ i ] );
            AE3D_CHECK_VULKAN( err, "vkCreateFramebuffer" );
        }
    }

    VkSampleCountFlagBits GetSampleBits( int msaaSampleCount )
    {
        if (msaaSampleCount == 4)
        {
            return VK_SAMPLE_COUNT_4_BIT;
        }
        else if (msaaSampleCount == 8)
        {
            return VK_SAMPLE_COUNT_8_BIT;
        }
        else if (msaaSampleCount == 16)
        {
            return VK_SAMPLE_COUNT_16_BIT;
        }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    void CreateRenderPassNonMSAA()
    {
        VkAttachmentDescription attachments[ 2 ];
        attachments[ 0 ].format = GfxDeviceGlobal::colorFormat;
        attachments[ 0 ].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[ 0 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[ 0 ].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[ 0 ].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[ 0 ].flags = 0;

        attachments[ 1 ].format = GfxDeviceGlobal::depthFormat;
        attachments[ 1 ].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[ 1 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[ 1 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[ 1 ].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
        renderPassInfo.pNext = nullptr;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 0;
        renderPassInfo.pDependencies = nullptr;

        VkResult err = vkCreateRenderPass( GfxDeviceGlobal::device, &renderPassInfo, nullptr, &GfxDeviceGlobal::renderPass );
        AE3D_CHECK_VULKAN( err, "vkCreateRenderPass" );   
    }

    void CreateRenderPassMSAA()
    {
        VkAttachmentDescription attachments[ 4 ];

        // Multisampled attachment that we render to
        attachments[ 0 ].format = GfxDeviceGlobal::colorFormat;
        attachments[ 0 ].samples = GfxDeviceGlobal::msaaSampleBits;
        attachments[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[ 0 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[ 0 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[ 0 ].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[ 0 ].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[ 0 ].flags = 0;

        // This is the frame buffer attachment to where the multisampled image
        // will be resolved to and which will be presented to the swapchain.
        attachments[ 1 ].format = GfxDeviceGlobal::colorFormat;
        attachments[ 1 ].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[ 1 ].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[ 1 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[ 1 ].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[ 1 ].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[ 1 ].flags = 0;

        // Multisampled depth attachment we render to
        attachments[ 2 ].format = GfxDeviceGlobal::depthFormat;
        attachments[ 2 ].samples = GfxDeviceGlobal::msaaSampleBits;
        attachments[ 2 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[ 2 ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[ 2 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[ 2 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[ 2 ].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[ 2 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[ 2 ].flags = 0;

        // Depth resolve attachment
        attachments[ 3 ].format = GfxDeviceGlobal::depthFormat;
        attachments[ 3 ].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[ 3 ].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[ 3 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[ 3 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[ 3 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[ 3 ].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[ 3 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[ 3 ].flags = 0;

        VkAttachmentReference colorReference = {};
        colorReference.attachment = 0;
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthReference = {};
        depthReference.attachment = 2;
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Two resolve attachment references for color and depth
        VkAttachmentReference resolveReferences[ 2 ] = {};
        resolveReferences[ 0 ].attachment = 1;
        resolveReferences[ 0 ].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        resolveReferences[ 1 ].attachment = 3;
        resolveReferences[ 1 ].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorReference;
        // Pass our resolve attachments to the sub pass
        subpass.pResolveAttachments = &resolveReferences[ 0 ];
        subpass.pDepthStencilAttachment = &depthReference;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 4;
        renderPassInfo.pAttachments = &attachments[ 0 ];
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkResult err = vkCreateRenderPass( GfxDeviceGlobal::device, &renderPassInfo, nullptr, &GfxDeviceGlobal::renderPass );
        AE3D_CHECK_VULKAN( err, "vkCreateRenderPass" );
    }

    void CreateDepthStencil()
    {
        VkImageCreateInfo image = {};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.pNext = nullptr;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = GfxDeviceGlobal::depthFormat;
        image.extent = { static_cast< std::uint32_t >( WindowGlobal::windowWidth ), static_cast< std::uint32_t >( WindowGlobal::windowHeight ), 1 };
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = GfxDeviceGlobal::msaaSampleBits;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        image.flags = 0;

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.pNext = nullptr;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;

        VkImageViewCreateInfo depthStencilView = {};
        depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthStencilView.pNext = nullptr;
        depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthStencilView.format = GfxDeviceGlobal::depthFormat;
        depthStencilView.flags = 0;
        depthStencilView.subresourceRange = {};
        depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        depthStencilView.subresourceRange.baseMipLevel = 0;
        depthStencilView.subresourceRange.levelCount = 1;
        depthStencilView.subresourceRange.baseArrayLayer = 0;
        depthStencilView.subresourceRange.layerCount = 1;

        VkResult err = vkCreateImage( GfxDeviceGlobal::device, &image, nullptr, &GfxDeviceGlobal::depthStencil.image );
        AE3D_CHECK_VULKAN( err, "depth stencil" );

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.image, &memReqs );
        mem_alloc.allocationSize = memReqs.size;
        GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &mem_alloc, nullptr, &GfxDeviceGlobal::depthStencil.mem );
        AE3D_CHECK_VULKAN( err, "depth stencil memory" );

        err = vkBindImageMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.image, GfxDeviceGlobal::depthStencil.mem, 0 );
        AE3D_CHECK_VULKAN( err, "depth stencil memory" );
        SetImageLayout( GfxDeviceGlobal::setupCmdBuffer, GfxDeviceGlobal::depthStencil.image, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 0, 1 );

        depthStencilView.image = GfxDeviceGlobal::depthStencil.image;
        err = vkCreateImageView( GfxDeviceGlobal::device, &depthStencilView, nullptr, &GfxDeviceGlobal::depthStencil.view );
        AE3D_CHECK_VULKAN( err, "depth stencil view" );
    }

    void FlushSetupCommandBuffer()
    {
        System::Assert( GfxDeviceGlobal::graphicsQueue != VK_NULL_HANDLE, "graphics queue not initialized" );

        if (GfxDeviceGlobal::setupCmdBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkResult err = vkEndCommandBuffer( GfxDeviceGlobal::setupCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &GfxDeviceGlobal::setupCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
        AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );

        err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
        AE3D_CHECK_VULKAN( err, "vkQueueWaitIdle" );

        vkFreeCommandBuffers( GfxDeviceGlobal::device, GfxDeviceGlobal::cmdPool, 1, &GfxDeviceGlobal::setupCmdBuffer );
        GfxDeviceGlobal::setupCmdBuffer = VK_NULL_HANDLE;
    }

    void CreateDescriptorPool()
    {
        VkDescriptorPoolSize typeCounts[ 2 ];
        typeCounts[ 0 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        typeCounts[ 0 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 1 ].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        typeCounts[ 1 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.pNext = nullptr;
        descriptorPoolInfo.poolSizeCount = 2;
        descriptorPoolInfo.pPoolSizes = typeCounts;
        descriptorPoolInfo.maxSets = AE3D_DESCRIPTOR_SETS_COUNT;
        descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkResult err = vkCreateDescriptorPool( GfxDeviceGlobal::device, &descriptorPoolInfo, nullptr, &GfxDeviceGlobal::descriptorPool );
        AE3D_CHECK_VULKAN( err, "vkCreateDescriptorPool" );

        GfxDeviceGlobal::descriptorSets.resize( AE3D_DESCRIPTOR_SETS_COUNT );

        for (std::size_t i = 0; i < GfxDeviceGlobal::descriptorSets.size(); ++i)
        {
            VkDescriptorSetAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = GfxDeviceGlobal::descriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &GfxDeviceGlobal::descriptorSetLayout;

            err = vkAllocateDescriptorSets( GfxDeviceGlobal::device, &allocInfo, &GfxDeviceGlobal::descriptorSets[ i ] );
            AE3D_CHECK_VULKAN( err, "vkAllocateDescriptorSets" );
        }
    }

    VkDescriptorSet AllocateDescriptorSet( const VkDescriptorBufferInfo& uboDesc, const VkImageView& view, VkSampler sampler )
    {
        VkDescriptorSet outDescriptorSet = GfxDeviceGlobal::descriptorSets[ GfxDeviceGlobal::descriptorSetIndex ];
        GfxDeviceGlobal::descriptorSetIndex = (GfxDeviceGlobal::descriptorSetIndex + 1) % GfxDeviceGlobal::descriptorSets.size();

        if (GfxDeviceGlobal::descriptorSetIndex >= static_cast<int>(GfxDeviceGlobal::descriptorSets.size()))
        {
            System::Print( "Too many descriptor sets: %d, max %u\n", GfxDeviceGlobal::descriptorSetIndex, GfxDeviceGlobal::descriptorSets.size() );
        }

        // Binding 0 : Uniform buffer
        VkWriteDescriptorSet uboSet = {};
        uboSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uboSet.dstSet = outDescriptorSet;
        uboSet.descriptorCount = 1;
        uboSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboSet.pBufferInfo = &uboDesc;
        uboSet.dstBinding = 0;

        // Binding 1 : Sampler
        VkDescriptorImageInfo samplerDesc = {};
        samplerDesc.sampler = sampler;
        samplerDesc.imageView = view;
        samplerDesc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet samplerSet = {};
        samplerSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerSet.dstSet = outDescriptorSet;
        samplerSet.descriptorCount = 1;
        samplerSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerSet.pImageInfo = &samplerDesc;
        samplerSet.dstBinding = 1;

        VkWriteDescriptorSet sets[ 2 ] = { uboSet, samplerSet };
        vkUpdateDescriptorSets( GfxDeviceGlobal::device, 2, sets, 0, nullptr );

        return outDescriptorSet;
    }

    void CreateDescriptorSetLayout()
    {
        // Binding 0 : Uniform buffer (Vertex shader)
        VkDescriptorSetLayoutBinding layoutBindingUBO = {};
        layoutBindingUBO.binding = 0;
        layoutBindingUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBindingUBO.descriptorCount = 1;
        layoutBindingUBO.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBindingUBO.pImmutableSamplers = nullptr;

        // Binding 1 : Sampler (Fragment shader)
        VkDescriptorSetLayoutBinding layoutBindingSampler = {};
        layoutBindingSampler.binding = 1;
        layoutBindingSampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindingSampler.descriptorCount = 1;
        layoutBindingSampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindingSampler.pImmutableSamplers = nullptr;

        const VkDescriptorSetLayoutBinding bindings[2] = { layoutBindingUBO, layoutBindingSampler };

        VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
        descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayout.pNext = nullptr;
        descriptorLayout.bindingCount = 2;
        descriptorLayout.pBindings = bindings;

        VkResult err = vkCreateDescriptorSetLayout( GfxDeviceGlobal::device, &descriptorLayout, nullptr, &GfxDeviceGlobal::descriptorSetLayout );
        AE3D_CHECK_VULKAN( err, "vkCreateDescriptorSetLayout" );

        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &GfxDeviceGlobal::descriptorSetLayout;

        err = vkCreatePipelineLayout( GfxDeviceGlobal::device, &pPipelineLayoutCreateInfo, nullptr, &GfxDeviceGlobal::pipelineLayout );
        AE3D_CHECK_VULKAN( err, "vkCreatePipelineLayout" );
    }

    void CreateRenderer( int samples )
    {
        GfxDeviceGlobal::msaaSampleBits = GetSampleBits( samples );
        CreateInstance( &GfxDeviceGlobal::instance );
        
        if (debug::enabled)
        {
            debug::Setup( GfxDeviceGlobal::instance );
        }

        CreateDevice();

        if (debug::enabled)
        {
            debug::SetupDevice( GfxDeviceGlobal::device );
        }

        std::vector< VkFormat > depthFormats = { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
        bool depthFormatFound = false;

        for (auto& format : depthFormats)
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties( GfxDeviceGlobal::physicalDevice, format, &formatProps );
            // Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                GfxDeviceGlobal::depthFormat = format;
                depthFormatFound = true;
                break;
            }
        }

        System::Assert( depthFormatFound, "No suitable depth format found" );

        LoadFunctionPointers();
        InitSwapChain();
        AllocateSetupCommandBuffer();
        SetupSwapChain();
        AllocateCommandBuffers();
        CreateDepthStencil();

        if (samples > 1)
        {
            CreateRenderPassMSAA();
            CreateMsaaColor();
            CreateMsaaDepth();

            SetImageLayout( GfxDeviceGlobal::setupCmdBuffer, GfxDeviceGlobal::msaaTarget.colorImage, VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 0, 1 );
            SetImageLayout( GfxDeviceGlobal::setupCmdBuffer, GfxDeviceGlobal::msaaTarget.depthImage, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 0, 1 );
        }
        else
        {
            CreateRenderPassNonMSAA();
        }

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        VkResult err = vkCreatePipelineCache( GfxDeviceGlobal::device, &pipelineCacheCreateInfo, nullptr, &GfxDeviceGlobal::pipelineCache );
        AE3D_CHECK_VULKAN( err, "vkCreatePipelineCache" );

        if (samples > 1)
        {
            CreateFramebufferMSAA();
        }
        else
        {
            CreateFramebufferNonMSAA();
        }

        FlushSetupCommandBuffer();
        CreateDescriptorSetLayout();
        CreateDescriptorPool();

        VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {};
        presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        presentCompleteSemaphoreCreateInfo.pNext = nullptr;

        err = vkCreateSemaphore( GfxDeviceGlobal::device, &presentCompleteSemaphoreCreateInfo, nullptr, &GfxDeviceGlobal::presentCompleteSemaphore );
        AE3D_CHECK_VULKAN( err, "vkCreateSemaphore" );

        VkSemaphoreCreateInfo renderCompleteSemaphoreCreateInfo = {};
        renderCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        renderCompleteSemaphoreCreateInfo.pNext = nullptr;

        err = vkCreateSemaphore( GfxDeviceGlobal::device, &renderCompleteSemaphoreCreateInfo, nullptr, &GfxDeviceGlobal::renderCompleteSemaphore );
        AE3D_CHECK_VULKAN( err, "vkCreateSemaphore" );

        GfxDevice::SetClearColor( 0, 0, 0 );
    }
}

void ae3d::GfxDevice::SetPolygonOffset( bool, float, float )
{
}

void ae3d::GfxDevice::PushGroupMarker( const char* name )
{
    debug::BeginRegion( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], name, 0, 1, 0 );
}

void ae3d::GfxDevice::PopGroupMarker()
{
    debug::EndRegion( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ] );
}

void ae3d::GfxDevice::BeginRenderPassAndCommandBuffer()
{
    VkResult res = vkDeviceWaitIdle( GfxDeviceGlobal::device );
    AE3D_CHECK_VULKAN( res, "vkDeviceWaitIdle" );

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;

    std::vector< VkClearValue > clearValues;
    if (GfxDeviceGlobal::msaaSampleBits != VK_SAMPLE_COUNT_1_BIT)
    {
        clearValues.resize( 3 );
        clearValues[ 0 ].color = GfxDeviceGlobal::clearColor;
        clearValues[ 1 ].color = GfxDeviceGlobal::clearColor;
        clearValues[ 2 ].depthStencil = { 1.0f, 0 };
    }
    else
    {
        clearValues.resize( 2 );
        clearValues[ 0 ].color = GfxDeviceGlobal::clearColor;
        clearValues[ 1 ].depthStencil = { 1.0f, 0 };
    }

    const uint32_t width = GfxDeviceGlobal::renderTexture0 ? GfxDeviceGlobal::renderTexture0->GetWidth() : WindowGlobal::windowWidth;
    const uint32_t height = GfxDeviceGlobal::renderTexture0 ? GfxDeviceGlobal::renderTexture0->GetHeight() : WindowGlobal::windowHeight;

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = GfxDeviceGlobal::renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = (std::uint32_t)clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();
    renderPassBeginInfo.framebuffer = GfxDeviceGlobal::frameBuffer0 != VK_NULL_HANDLE ? GfxDeviceGlobal::frameBuffer0 : 
                                      GfxDeviceGlobal::frameBuffers[ GfxDeviceGlobal::currentBuffer ];

    VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

    vkCmdBeginRenderPass( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

    VkViewport viewport = {};
    viewport.height = (float)height;
    viewport.width = (float)width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    vkCmdSetViewport( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], 0, 1, &viewport );

    VkRect2D scissor = {};
    scissor.extent.width = width;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], 0, 1, &scissor );
}

void ae3d::GfxDevice::EndRenderPassAndCommandBuffer()
{
    vkCmdEndRenderPass( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ] );

    VkResult err = vkEndCommandBuffer( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ] );
    AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );

    VkResult res = vkDeviceWaitIdle( GfxDeviceGlobal::device );
    AE3D_CHECK_VULKAN( res, "vkDeviceWaitIdle" );
}

void ae3d::GfxDevice::SetClearColor( float red, float green, float blue )
{
    GfxDeviceGlobal::clearColor.float32[ 0 ] = red;
    GfxDeviceGlobal::clearColor.float32[ 1 ] = green;
    GfxDeviceGlobal::clearColor.float32[ 2 ] = blue;
    GfxDeviceGlobal::clearColor.float32[ 3 ] = 1.0f;
}

void ae3d::GfxDevice::ResetFrameStatistics()
{
    Stats::drawCalls = 0;
    Stats::barrierCalls = 0;
    Stats::fenceCalls = 0;
    Stats::startFrameTimePoint = std::chrono::high_resolution_clock::now();
}

void ae3d::GfxDevice::Set_sRGB_Writes( bool /*enable*/ )
{
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return Stats::drawCalls;
}

int ae3d::GfxDevice::GetTextureBinds()
{
    return 0;
}

int ae3d::GfxDevice::GetRenderTargetBinds()
{
    return 0;
}

int ae3d::GfxDevice::GetVertexBufferBinds()
{
    return 0;
}

int ae3d::GfxDevice::GetShaderBinds()
{
    return 0;
}

void ae3d::GfxDevice::GetGpuMemoryUsage( unsigned& outUsedMBytes, unsigned& outBudgetMBytes )
{
    outUsedMBytes = 0;
    outBudgetMBytes = 0;
}

int ae3d::GfxDevice::GetBarrierCalls()
{
    return Stats::barrierCalls;
}

int ae3d::GfxDevice::GetFenceCalls()
{
    return Stats::fenceCalls;
}

void ae3d::GfxDevice::Init( int /*width*/, int /*height*/ )
{
}

void ae3d::GfxDevice::IncDrawCalls()
{
    ++Stats::drawCalls;
}

void ae3d::GfxDevice::ClearScreen( unsigned /*clearFlags*/ )
{
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc,
                            CullMode cullMode )
{
    System::Assert( startIndex > -1 && startIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in startIndex" );
    System::Assert( endIndex > -1 && endIndex >= startIndex && endIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in endIndex" );
    System::Assert( GfxDeviceGlobal::currentBuffer < GfxDeviceGlobal::drawCmdBuffers.size(), "invalid draw buffer index" );
    System::Assert( GfxDeviceGlobal::pipelineLayout != VK_NULL_HANDLE, "invalid pipelineLayout" );

    if (GfxDeviceGlobal::view0 == VK_NULL_HANDLE || GfxDeviceGlobal::sampler0 == VK_NULL_HANDLE)
    {
        return;
    }

    if (shader.GetVertexInfo().module == VK_NULL_HANDLE || shader.GetFragmentInfo().module == VK_NULL_HANDLE)
    {
        return;
    }

    const unsigned psoHash = GetPSOHash( vertexBuffer, shader, blendMode, depthFunc, cullMode );

    if (GfxDeviceGlobal::psoCache.find( psoHash ) == std::end( GfxDeviceGlobal::psoCache ))
    {
        CreatePSO( vertexBuffer, shader, blendMode, depthFunc, cullMode, psoHash );
    }

    VkDescriptorSet descriptorSet = AllocateDescriptorSet( GfxDeviceGlobal::frameUbos.back().uboDesc, GfxDeviceGlobal::view0, GfxDeviceGlobal::sampler0 );

    vkCmdBindDescriptorSets( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], VK_PIPELINE_BIND_POINT_GRAPHICS,
                             GfxDeviceGlobal::pipelineLayout, 0, 1, &descriptorSet, 0, nullptr );

    vkCmdBindPipeline( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], VK_PIPELINE_BIND_POINT_GRAPHICS, GfxDeviceGlobal::psoCache[ psoHash ] );

    VkDeviceSize offsets[ 1 ] = { 0 };
    vkCmdBindVertexBuffers( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], VertexBuffer::VERTEX_BUFFER_BIND_ID, 1, vertexBuffer.GetVertexBuffer(), offsets );

    vkCmdBindIndexBuffer( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], *vertexBuffer.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16 );
    vkCmdDrawIndexed( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], (endIndex - startIndex) * 3, 1, startIndex * 3, 0, 0 );
    IncDrawCalls();
}

void ae3d::GfxDevice::CreateNewUniformBuffer()
{
    const VkDeviceSize uboSize = 16 * 4;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = uboSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    Ubo ubo;

    VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &ubo.ubo );
    AE3D_CHECK_VULKAN( err, "vkCreateBuffer UBO" );

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, ubo.ubo, &memReqs );

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = memReqs.size;
    GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocInfo.memoryTypeIndex );
    err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &ubo.uboMemory );
    AE3D_CHECK_VULKAN( err, "vkAllocateMemory UBO" );

    err = vkBindBufferMemory( GfxDeviceGlobal::device, ubo.ubo, ubo.uboMemory, 0 );
    AE3D_CHECK_VULKAN( err, "vkBindBufferMemory UBO" );

    ubo.uboDesc.buffer = ubo.ubo;
    ubo.uboDesc.offset = 0;
    ubo.uboDesc.range = uboSize;

    err = vkMapMemory( GfxDeviceGlobal::device, ubo.uboMemory, 0, uboSize, 0, (void **)&ubo.uboData );
    AE3D_CHECK_VULKAN( err, "vkMapMemory UBO" );

    GfxDeviceGlobal::frameUbos.push_back( ubo );
}

std::uint8_t* ae3d::GfxDevice::GetCurrentUbo()
{
    ae3d::System::Assert( !GfxDeviceGlobal::frameUbos.empty() && GfxDeviceGlobal::frameUbos.back().uboData != nullptr, "no Ubo" );
    return GfxDeviceGlobal::frameUbos.back().uboData;
}

void ae3d::GfxDevice::ErrorCheck( const char* /*info*/ )
{
}

void ae3d::GfxDevice::BeginFrame()
{
    ae3d::System::Assert( acquireNextImageKHR != nullptr, "function pointers not loaded" );
    ae3d::System::Assert( GfxDeviceGlobal::swapChain != VK_NULL_HANDLE, "swap chain not initialized" );
    
    VkResult err = acquireNextImageKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, UINT64_MAX, GfxDeviceGlobal::presentCompleteSemaphore, (VkFence)nullptr, &GfxDeviceGlobal::currentBuffer );
    AE3D_CHECK_VULKAN( err, "acquireNextImage" );

    SubmitPostPresentBarrier();
}

void ae3d::GfxDevice::Present()
{
    VkPipelineStageFlags pipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &pipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &GfxDeviceGlobal::presentCompleteSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &GfxDeviceGlobal::renderCompleteSemaphore;

    VkResult err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );

    SubmitPrePresentBarrier();

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &GfxDeviceGlobal::swapChain;
    presentInfo.pImageIndices = &GfxDeviceGlobal::currentBuffer;
    presentInfo.pWaitSemaphores = &GfxDeviceGlobal::renderCompleteSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    err = queuePresentKHR( GfxDeviceGlobal::graphicsQueue, &presentInfo );
    AE3D_CHECK_VULKAN( err, "queuePresent" );

    err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
    AE3D_CHECK_VULKAN( err, "vkQueueWaitIdle" );

    for (std::size_t i = 0; i < GfxDeviceGlobal::pendingFreeVBs.size(); ++i)
    {
        vkDestroyBuffer( GfxDeviceGlobal::device, GfxDeviceGlobal::pendingFreeVBs[ i ], nullptr );
    }

    for (std::size_t i = 0; i < GfxDeviceGlobal::frameUbos.size(); ++i)
    {
        vkFreeMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::frameUbos[ i ].uboMemory, nullptr );
        vkDestroyBuffer( GfxDeviceGlobal::device, GfxDeviceGlobal::frameUbos[ i ].ubo, nullptr );
    }

    GfxDeviceGlobal::pendingFreeVBs.clear();
    GfxDeviceGlobal::frameUbos.clear();
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    VkResult err = vkDeviceWaitIdle( GfxDeviceGlobal::device );
	AE3D_CHECK_VULKAN( err, "vkDeviceWaitIdle" );

    debug::Free( GfxDeviceGlobal::instance );
    
    for (std::size_t i = 0; i < GfxDeviceGlobal::swapchainBuffers.size(); ++i)
    {
        vkDestroyImageView( GfxDeviceGlobal::device, GfxDeviceGlobal::swapchainBuffers[ i ].view, nullptr );
    }

    for (std::size_t i = 0; i < GfxDeviceGlobal::frameBuffers.size(); ++i)
    {
        vkDestroyFramebuffer( GfxDeviceGlobal::device, GfxDeviceGlobal::frameBuffers[ i ], nullptr );
    }

    vkDestroyImage( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.image, nullptr );
    vkDestroyImageView( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.view, nullptr );

    if (GfxDeviceGlobal::msaaTarget.colorImage != VK_NULL_HANDLE)
    {
        vkDestroyImage( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.colorImage, nullptr );
        vkDestroyImage( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.depthImage, nullptr );
        vkDestroyImageView( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.colorView, nullptr );
        vkDestroyImageView( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.depthView, nullptr );
    }

    Shader::DestroyShaders();
    Texture2D::DestroyTextures();
    TextureCube::DestroyTextures();
    RenderTexture::DestroyTextures();
    VertexBuffer::DestroyBuffers();

    vkDestroySemaphore( GfxDeviceGlobal::device, GfxDeviceGlobal::renderCompleteSemaphore, nullptr );
    vkDestroySemaphore( GfxDeviceGlobal::device, GfxDeviceGlobal::presentCompleteSemaphore, nullptr );
    vkDestroyPipelineLayout( GfxDeviceGlobal::device, GfxDeviceGlobal::pipelineLayout, nullptr );
    vkDestroySwapchainKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, nullptr );
    vkDestroyCommandPool( GfxDeviceGlobal::device, GfxDeviceGlobal::cmdPool, nullptr );
    vkDestroyDevice( GfxDeviceGlobal::device, nullptr );
    vkDestroyInstance( GfxDeviceGlobal::instance, nullptr );
}

void ae3d::GfxDevice::SetRenderTarget( RenderTexture* target, unsigned /*cubeMapFace*/ )
{
    GfxDeviceGlobal::renderTexture0 = target;
    GfxDeviceGlobal::frameBuffer0 = target ? target->GetFrameBuffer() : VK_NULL_HANDLE;
}

void ae3d::GfxDevice::SetMultiSampling( bool /*enable*/ )
{
}
