#include "GfxDevice.hpp"
#include <cstdint>
#include <map>
#include <vector> 
#include <string>
#include <vulkan/vulkan.h>
#include "System.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#if VK_USE_PLATFORM_WIN32_KHR
#include <Windows.h>
#endif
#if VK_USE_PLATFORM_XCB_KHR
#include <X11/Xlib-xcb.h>
#endif

// Current implementation loosely based on samples by Sascha Willems - https://github.com/SaschaWillems/Vulkan, licensed under MIT license

PFN_vkCreateSwapchainKHR createSwapchainKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR getPhysicalDeviceSurfacePresentModesKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR getPhysicalDeviceSurfaceSupportKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR getPhysicalDeviceSurfaceFormatsKHR = nullptr;
PFN_vkGetSwapchainImagesKHR getSwapchainImagesKHR = nullptr;
PFN_vkAcquireNextImageKHR acquireNextImageKHR = nullptr;
PFN_vkQueuePresentKHR queuePresentKHR = nullptr;

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
    
    struct Samplers
    {
        VkSampler linearRepeat = VK_NULL_HANDLE;
    } samplers;
    
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkClearColorValue clearColor;
    std::vector< VkCommandBuffer > drawCmdBuffers;
    VkCommandBuffer setupCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer prePresentCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer postPresentCmdBuffer = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;
    VkFormat colorFormat;
    VkFormat depthFormat;
    VkColorSpaceKHR colorSpace;
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
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
    std::uint32_t queueNodeIndex = UINT32_MAX;
    std::uint32_t currentBuffer = 0;
    ae3d::Texture2D* texture2d0 = nullptr;
    ae3d::TextureCube* textureCube0 = nullptr;
    std::vector< VkDescriptorSet > pendingFreeDescriptorSets;
    std::vector< VkBuffer > pendingFreeVBs;
    int drawCalls = 0;
}

namespace debug
{
    bool enabled = false; // Disable when using RenderDoc.
    const int validationLayerCount = 9;
    const char *validationLayerNames[] =
    {
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_LUNARG_mem_tracker",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_draw_state",
        "VK_LAYER_LUNARG_param_checker",
        "VK_LAYER_LUNARG_swapchain",
        "VK_LAYER_LUNARG_device_limits",
        "VK_LAYER_LUNARG_image",
        "VK_LAYER_GOOGLE_unique_objects",
    };
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = nullptr;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = nullptr;
    PFN_vkDebugReportMessageEXT dbgBreakCallback = nullptr;

    VkDebugReportCallbackEXT debugReportCallback = nullptr;

    VkBool32 messageCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objType,
        uint64_t srcObject,
        size_t location,
        int32_t msgCode,
        const char* pLayerPrefix,
        const char* pMsg,
        void* pUserData )
    {
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            ae3d::System::Print( "Vulkan error: [%s], code: %d: %s\n", pLayerPrefix, msgCode, pMsg );
            ae3d::System::Assert( false, "Vulkan error" );
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            ae3d::System::Print( "Vulkan warning: [%s], code: %d: %s\n", pLayerPrefix, msgCode, pMsg );
#if _MSC_VER && _DEBUG
            __debugbreak();
#endif
        }

        return VK_FALSE;
    }

    void Setup( VkInstance instance )
    {
        CreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr( instance, "vkCreateDebugReportCallbackEXT" );
        DestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr( instance, "vkDestroyDebugReportCallbackEXT" );
        dbgBreakCallback = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr( instance, "vkDebugReportMessageEXT" );
        ae3d::System::Assert( CreateDebugReportCallback != nullptr, "CreateDebugReportCallback" );

        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.pNext = nullptr;
        dbgCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)messageCallback;
        dbgCreateInfo.pUserData = nullptr;
        dbgCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        VkResult err = CreateDebugReportCallback( instance, &dbgCreateInfo, nullptr, &debugReportCallback );
        ae3d::System::Assert( err == VK_SUCCESS, "Unable to create debug report callback" );
    }

    void Free( VkInstance instance )
    {
        if (enabled)
        {
            DestroyDebugReportCallback( instance, debugReportCallback, nullptr );
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
    void CheckVulkanResult( VkResult result, const char* message )
    {
        if (result != VK_SUCCESS)
        {
            System::Print( "Vulkan call failed. Context: %s\n", message );
            System::Assert( false, "Vulkan call failed" );
        }
    }

    // FIXME: duplicated in d3d12
    unsigned GetHash( const char* s, unsigned length )
    {
        const unsigned A = 54059;
        const unsigned B = 76963;

        unsigned h = 31;
        unsigned i = 0;

        while (i < length)
        {
            h = (h * A) ^ (s[ 0 ] * B);
            ++s;
            ++i;
        }

        return h;
    }

    unsigned GetPSOHash( ae3d::VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc )
    {
        std::string hashString;
        hashString += std::to_string( (ptrdiff_t)&vertexBuffer );
        hashString += std::to_string( (ptrdiff_t)&shader.GetVertexInfo().module );
        hashString += std::to_string( (ptrdiff_t)&shader.GetFragmentInfo().module );
        hashString += std::to_string( (unsigned)blendMode );
        hashString += std::to_string( ((unsigned)depthFunc) + 4 );

        return GetHash( hashString.c_str(), static_cast< unsigned >(hashString.length()) );
    }

    void CreateSamplers()
    {
        System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );

        VkSamplerCreateInfo sampler = {};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.pNext = nullptr;
        sampler.magFilter = VK_FILTER_LINEAR;
        sampler.minFilter = VK_FILTER_LINEAR;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.mipLodBias = 0.0f;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0.0f;
        // Max level-of-detail should match mip level count
        sampler.maxLod = 0;
        sampler.maxAnisotropy = 8;
        sampler.anisotropyEnable = VK_TRUE;
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VkResult err = vkCreateSampler( GfxDeviceGlobal::device, &sampler, nullptr, &GfxDeviceGlobal::samplers.linearRepeat );
        CheckVulkanResult( err, "vkCreateSampler" );
    }

    void CreatePSO( VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc )
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
        inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterizationState = {};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.cullMode = VK_CULL_MODE_NONE;
        rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationState.depthClampEnable = VK_FALSE;
        rasterizationState.rasterizerDiscardEnable = VK_FALSE;
        rasterizationState.depthBiasEnable = VK_FALSE;

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
        std::vector<VkDynamicState> dynamicStateEnables;
        dynamicStateEnables.push_back( VK_DYNAMIC_STATE_VIEWPORT );
        dynamicStateEnables.push_back( VK_DYNAMIC_STATE_SCISSOR );
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.pDynamicStates = dynamicStateEnables.data();
        dynamicState.dynamicStateCount = (std::uint32_t)dynamicStateEnables.size();

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
        multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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
        CheckVulkanResult( err, "vkCreateGraphicsPipelines" );

        const unsigned hash = GetPSOHash( vertexBuffer, shader, blendMode, depthFunc );
        GfxDeviceGlobal::psoCache[ hash ] = pso;
    }

    VkBool32 GetMemoryType( std::uint32_t typeBits, VkFlags properties, std::uint32_t* typeIndex )
    {
        for (std::uint32_t i = 0; i < 32; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((GfxDeviceGlobal::deviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties) == properties)
                {
                    *typeIndex = i;
                    return VK_TRUE;
                }
            }
            typeBits >>= 1;
        }
        return VK_FALSE;
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
        CheckVulkanResult( err, "vkAllocateCommandBuffers" );

        commandBufferAllocateInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::postPresentCmdBuffer );
        CheckVulkanResult( err, "vkAllocateCommandBuffers" );

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::prePresentCmdBuffer );
        CheckVulkanResult( err, "vkAllocateCommandBuffers" );
    }

    void SetImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout )
    {
        System::Assert( cmdbuffer != VK_NULL_HANDLE, "command buffer not initialized" );

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.pNext = nullptr;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageMemoryBarrier.oldLayout = oldImageLayout;
        imageMemoryBarrier.newLayout = newImageLayout;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = 1;
        imageMemoryBarrier.subresourceRange.layerCount = 1;

        if (oldImageLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)//VK_IMAGE_LAYOUT_UNDEFINED)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }

        if (oldImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        
        if (oldImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        }

        if (oldImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        }

        if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        }

        if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = imageMemoryBarrier.srcAccessMask | VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        }

        if (newImageLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        }

        if (newImageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        if (newImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        }

        const VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        const VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        vkCmdPipelineBarrier(
            cmdbuffer,
            srcStageFlags,
            destStageFlags,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier );
    }

    void SubmitPrePresentBarrier()
    {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::prePresentCmdBuffer, &cmdBufInfo );
        CheckVulkanResult( err, "vkBeginCommandBuffer" );

        VkImageMemoryBarrier prePresentBarrier = {};
        prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        prePresentBarrier.pNext = nullptr;
        prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        prePresentBarrier.dstAccessMask = 0;
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

        err = vkEndCommandBuffer( GfxDeviceGlobal::prePresentCmdBuffer );
        CheckVulkanResult( err, "vkEndCommandBuffer" );

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &GfxDeviceGlobal::prePresentCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
        CheckVulkanResult( err, "vkQueueSubmit" );
    }

    void SubmitPostPresentBarrier()
    {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::postPresentCmdBuffer, &cmdBufInfo );
        CheckVulkanResult( err, "vkBeginCommandBuffer" );

        VkImageMemoryBarrier postPresentBarrier = {};
        postPresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        postPresentBarrier.pNext = nullptr;
        postPresentBarrier.srcAccessMask = 0;
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

        err = vkEndCommandBuffer( GfxDeviceGlobal::postPresentCmdBuffer );
        CheckVulkanResult( err, "vkEndCommandBuffer" );

        VkSubmitInfo submitPostInfo = {};
        submitPostInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitPostInfo.commandBufferCount = 1;
        submitPostInfo.pCommandBuffers = &GfxDeviceGlobal::postPresentCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitPostInfo, VK_NULL_HANDLE );
        CheckVulkanResult( err, "vkQueueSubmit" );
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
        CheckVulkanResult( err, "vkAllocateCommandBuffers" );

        VkCommandBufferAllocateInfo info = {};
        info.commandBufferCount = 1;
        info.commandPool = GfxDeviceGlobal::cmdPool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.pNext = nullptr;
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &info, &GfxDeviceGlobal::setupCmdBuffer );
        CheckVulkanResult( err, "vkAllocateCommandBuffers" );

        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        err = vkBeginCommandBuffer( GfxDeviceGlobal::setupCmdBuffer, &cmdBufInfo );
        CheckVulkanResult( err, "vkBeginCommandBuffer" );
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
        CheckVulkanResult( err, "create surface" );
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

        // Search for a graphics and a present queue in the array of queue
        // families, try to find one that supports both
        std::uint32_t graphicsQueueNodeIndex = UINT32_MAX;
        std::uint32_t presentQueueNodeIndex = UINT32_MAX;

        for (std::uint32_t i = 0; i < queueCount; ++i)
        {
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
            System::Assert( false, "graphics and present queue have the same index" );
        }

        GfxDeviceGlobal::queueNodeIndex = graphicsQueueNodeIndex;

        std::uint32_t formatCount;
        err = getPhysicalDeviceSurfaceFormatsKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &formatCount, nullptr );
        CheckVulkanResult( err, "getPhysicalDeviceSurfaceFormatsKHR" );

        std::vector< VkSurfaceFormatKHR > surfFormats( formatCount );
        err = getPhysicalDeviceSurfaceFormatsKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &formatCount, surfFormats.data() );
        CheckVulkanResult( err, "getPhysicalDeviceSurfaceFormatsKHR" );

        if (formatCount == 1 && surfFormats[ 0 ].format == VK_FORMAT_UNDEFINED)
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
        CheckVulkanResult( err, "getPhysicalDeviceSurfaceCapabilitiesKHR" );

        std::uint32_t presentModeCount = 0;
        err = getPhysicalDeviceSurfacePresentModesKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &presentModeCount, nullptr );
        CheckVulkanResult( err, "getPhysicalDeviceSurfacePresentModesKHR" );

        std::vector< VkPresentModeKHR > presentModes( presentModeCount );
        err = getPhysicalDeviceSurfacePresentModesKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &presentModeCount, presentModes.data() );
        CheckVulkanResult( err, "getPhysicalDeviceSurfacePresentModesKHR" );

        VkExtent2D swapchainExtent = {};
        if (surfCaps.currentExtent.width == -1)
        {
            System::Print("Setting swapchain dimension to %dx%d\n", WindowGlobal::windowWidth, WindowGlobal::windowHeight );
            swapchainExtent.width = WindowGlobal::windowWidth;
            swapchainExtent.height = WindowGlobal::windowHeight;
        }
        else
        {
            swapchainExtent = surfCaps.currentExtent;
            WindowGlobal::windowWidth = surfCaps.currentExtent.width;
            WindowGlobal::windowHeight = surfCaps.currentExtent.height;
        }

        // Tries to use mailbox mode, low latency and non-tearing
        VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (size_t i = 0; i < presentModeCount; ++i)
        {
            if (presentModes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[ i ] == VK_PRESENT_MODE_IMMEDIATE_KHR))
            {
                swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
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
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.pQueueFamilyIndices = nullptr;
        swapchainInfo.presentMode = swapchainPresentMode;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        err = createSwapchainKHR( GfxDeviceGlobal::device, &swapchainInfo, nullptr, &GfxDeviceGlobal::swapChain );
        CheckVulkanResult( err, "swapchain" );

        std::uint32_t imageCount;
        err = getSwapchainImagesKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, &imageCount, nullptr );
        CheckVulkanResult( err, "getSwapchainImagesKHR" );

        GfxDeviceGlobal::swapchainImages.resize( imageCount );

        err = getSwapchainImagesKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, &imageCount, GfxDeviceGlobal::swapchainImages.data() );
        CheckVulkanResult( err, "getSwapchainImagesKHR" );

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
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR );
            
            colorAttachmentView.image = GfxDeviceGlobal::swapchainBuffers[ i ].image;

            err = vkCreateImageView( GfxDeviceGlobal::device, &colorAttachmentView, nullptr, &GfxDeviceGlobal::swapchainBuffers[ i ].view );
            CheckVulkanResult( err, "vkCreateImageView" );
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

    void CreateInstance()
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Aether3D";
        appInfo.pEngineName = "Aether3D";
        appInfo.apiVersion = VK_MAKE_VERSION( 1, 0, 2 );

        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = nullptr;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        VkResult result;

        if (debug::enabled)
        {
            instanceCreateInfo.enabledLayerCount = debug::validationLayerCount;
            instanceCreateInfo.ppEnabledLayerNames = debug::validationLayerNames;
#if VK_USE_PLATFORM_WIN32_KHR
            static const char* enabledExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
#endif
#if VK_USE_PLATFORM_XCB_KHR
            static const char* enabledExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME };
#endif
            instanceCreateInfo.enabledExtensionCount = 3;
            instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
            result = vkCreateInstance( &instanceCreateInfo, nullptr, &GfxDeviceGlobal::instance );
        }
        else
        {
#if VK_USE_PLATFORM_WIN32_KHR
            static const char* enabledExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
#endif
#if VK_USE_PLATFORM_XCB_KHR
            static const char* enabledExtensions[] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME };
#endif
            instanceCreateInfo.enabledExtensionCount = 2;
            instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
            result = vkCreateInstance( &instanceCreateInfo, nullptr, &GfxDeviceGlobal::instance );
        }

        CheckVulkanResult( result, "instance" );
    }

    void CreateDevice()
    {
        System::Assert( GfxDeviceGlobal::instance != VK_NULL_HANDLE, "instance not created yet" );

        std::uint32_t gpuCount;
        VkResult result = vkEnumeratePhysicalDevices( GfxDeviceGlobal::instance, &gpuCount, nullptr );
        CheckVulkanResult( result, "vkEnumeratePhysicalDevices" );
        if (gpuCount < 1)
        {
            System::Print( "Your system doesn't have Vulkan capable GPU.\n" );
        }

        result = vkEnumeratePhysicalDevices( GfxDeviceGlobal::instance, &gpuCount, &GfxDeviceGlobal::physicalDevice );
        CheckVulkanResult( result, "vkEnumeratePhysicalDevices" );

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

        const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = nullptr;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.enabledExtensionCount = 1;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
        
        if (debug::enabled)
        {
            deviceCreateInfo.enabledLayerCount = debug::validationLayerCount;
            deviceCreateInfo.ppEnabledLayerNames = debug::validationLayerNames;
        }

        result = vkCreateDevice( GfxDeviceGlobal::physicalDevice, &deviceCreateInfo, nullptr, &GfxDeviceGlobal::device );
        CheckVulkanResult( result, "device" );

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

    void CreateFramebuffer()
    {
        VkImageView attachments[ 2 ];

        // Depth/Stencil attachment is the same for all frame buffers
        attachments[ 1 ] = GfxDeviceGlobal::depthStencil.view;

        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.pNext = NULL;
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
            CheckVulkanResult( err, "vkCreateFramebuffer" );
        }
    }

    void CreateRenderPass()
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

        attachments[ 1 ].format = GfxDeviceGlobal::depthFormat;
        attachments[ 1 ].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[ 1 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[ 1 ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[ 1 ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[ 1 ].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments[ 1 ].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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
        CheckVulkanResult( err, "vkCreateRenderPass" );
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
        image.samples = VK_SAMPLE_COUNT_1_BIT;
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
        CheckVulkanResult( err, "depth stencil" );

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.image, &memReqs );
        mem_alloc.allocationSize = memReqs.size;
        GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &mem_alloc, nullptr, &GfxDeviceGlobal::depthStencil.mem );
        CheckVulkanResult( err, "depth stencil memory" );

        err = vkBindImageMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.image, GfxDeviceGlobal::depthStencil.mem, 0 );
        CheckVulkanResult( err, "depth stencil memory" );
        SetImageLayout( GfxDeviceGlobal::setupCmdBuffer, GfxDeviceGlobal::depthStencil.image, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );

        depthStencilView.image = GfxDeviceGlobal::depthStencil.image;
        err = vkCreateImageView( GfxDeviceGlobal::device, &depthStencilView, nullptr, &GfxDeviceGlobal::depthStencil.view );
        CheckVulkanResult( err, "depth stencil view" );
    }

    void FlushSetupCommandBuffer()
    {
        System::Assert( GfxDeviceGlobal::graphicsQueue != VK_NULL_HANDLE, "graphics queue not initialized" );

        if (GfxDeviceGlobal::setupCmdBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkResult err = vkEndCommandBuffer( GfxDeviceGlobal::setupCmdBuffer );
        CheckVulkanResult( err, "vkEndCommandBuffer" );

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &GfxDeviceGlobal::setupCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
        CheckVulkanResult( err, "vkQueueSubmit" );

        err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
        CheckVulkanResult( err, "vkQueueWaitIdle" );

        vkFreeCommandBuffers( GfxDeviceGlobal::device, GfxDeviceGlobal::cmdPool, 1, &GfxDeviceGlobal::setupCmdBuffer );
        GfxDeviceGlobal::setupCmdBuffer = VK_NULL_HANDLE;
    }

    void CreateDescriptorPool()
    {
        VkDescriptorPoolSize typeCounts[ 2 ];
        typeCounts[ 0 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        typeCounts[ 0 ].descriptorCount = 10;
        typeCounts[ 1 ].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        typeCounts[ 1 ].descriptorCount = 10;

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.pNext = nullptr;
        descriptorPoolInfo.poolSizeCount = 2;
        descriptorPoolInfo.pPoolSizes = typeCounts;
        descriptorPoolInfo.maxSets = 100;
        descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkResult err = vkCreateDescriptorPool( GfxDeviceGlobal::device, &descriptorPoolInfo, nullptr, &GfxDeviceGlobal::descriptorPool );
        CheckVulkanResult( err, "vkCreateDescriptorPool" );
    }

    VkDescriptorSet AllocateDescriptorSet( const VkDescriptorBufferInfo& uboDesc, const VkImageView& view )
    {
        ae3d::System::Assert( GfxDeviceGlobal::descriptorSetLayout != VK_NULL_HANDLE, "descriptor set not created" );
        ae3d::System::Assert( GfxDeviceGlobal::descriptorPool != VK_NULL_HANDLE, "descriptorPool not created" );
        ae3d::System::Assert( GfxDeviceGlobal::samplers.linearRepeat != VK_NULL_HANDLE, "linearRepeat not initted" );

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = GfxDeviceGlobal::descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &GfxDeviceGlobal::descriptorSetLayout;

        VkDescriptorSet outDescriptorSet;
        VkResult err = vkAllocateDescriptorSets( GfxDeviceGlobal::device, &allocInfo, &outDescriptorSet );
        CheckVulkanResult( err, "vkAllocateDescriptorSets" );

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
        samplerDesc.sampler = GfxDeviceGlobal::samplers.linearRepeat;
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
        CheckVulkanResult( err, "vkCreateDescriptorSetLayout" );

        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.pNext = nullptr;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &GfxDeviceGlobal::descriptorSetLayout;

        err = vkCreatePipelineLayout( GfxDeviceGlobal::device, &pPipelineLayoutCreateInfo, nullptr, &GfxDeviceGlobal::pipelineLayout );
        CheckVulkanResult( err, "vkCreatePipelineLayout" );
    }

    void CreateRenderer( int samples )
    {
        CreateInstance();
        
        if (debug::enabled)
        {
            debug::Setup( GfxDeviceGlobal::instance );
        }

        CreateDevice();

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
        CreateRenderPass();

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        VkResult err = vkCreatePipelineCache( GfxDeviceGlobal::device, &pipelineCacheCreateInfo, nullptr, &GfxDeviceGlobal::pipelineCache );
        CheckVulkanResult( err, "vkCreatePipelineCache" );

        CreateFramebuffer();
        FlushSetupCommandBuffer();
        CreateDescriptorSetLayout();
        CreateDescriptorPool();
        CreateSamplers();

        VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {};
        presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        presentCompleteSemaphoreCreateInfo.pNext = nullptr;

        err = vkCreateSemaphore( GfxDeviceGlobal::device, &presentCompleteSemaphoreCreateInfo, nullptr, &GfxDeviceGlobal::presentCompleteSemaphore );
        CheckVulkanResult( err, "vkCreateSemaphore" );

        VkSemaphoreCreateInfo renderCompleteSemaphoreCreateInfo = {};
        renderCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        renderCompleteSemaphoreCreateInfo.pNext = nullptr;

        err = vkCreateSemaphore( GfxDeviceGlobal::device, &renderCompleteSemaphoreCreateInfo, nullptr, &GfxDeviceGlobal::renderCompleteSemaphore );
        CheckVulkanResult( err, "vkCreateSemaphore" );

        GfxDevice::SetClearColor( 0, 0, 0 );
    }
}

void ae3d::GfxDevice::BeginRenderPassAndCommandBuffer()
{
    VkResult res = vkDeviceWaitIdle( GfxDeviceGlobal::device );
    CheckVulkanResult( res, "vkDeviceWaitIdle" );

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;

    VkClearValue clearValues[ 2 ];
    clearValues[ 0 ].color = GfxDeviceGlobal::clearColor;
    clearValues[ 1 ].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = GfxDeviceGlobal::renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = WindowGlobal::windowWidth;
    renderPassBeginInfo.renderArea.extent.height = WindowGlobal::windowHeight;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = GfxDeviceGlobal::frameBuffers[ GfxDeviceGlobal::currentBuffer ];

    VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], &cmdBufInfo );
    CheckVulkanResult( err, "vkBeginCommandBuffer" );

    vkCmdBeginRenderPass( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

    VkViewport viewport = {};
    viewport.height = (float)WindowGlobal::windowHeight;
    viewport.width = (float)WindowGlobal::windowWidth;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    vkCmdSetViewport( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], 0, 1, &viewport );

    VkRect2D scissor = {};
    scissor.extent.width = WindowGlobal::windowWidth;
    scissor.extent.height = WindowGlobal::windowHeight;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], 0, 1, &scissor );
}

void ae3d::GfxDevice::EndRenderPassAndCommandBuffer()
{
    vkCmdEndRenderPass( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ] );

    VkResult err = vkEndCommandBuffer( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ] );
    CheckVulkanResult( err, "vkEndCommandBuffer" );

    VkResult res = vkDeviceWaitIdle( GfxDeviceGlobal::device );
    CheckVulkanResult( res, "vkDeviceWaitIdle" );
}

void ae3d::GfxDevice::SetBackFaceCulling( bool enable )
{
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
    GfxDeviceGlobal::drawCalls = 0;
}

void ae3d::GfxDevice::Set_sRGB_Writes( bool /*enable*/ )
{
}

int ae3d::GfxDevice::GetDrawCalls()
{
    return GfxDeviceGlobal::drawCalls;
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

void ae3d::GfxDevice::Init( int width, int height )
{
}

void ae3d::GfxDevice::IncDrawCalls()
{
    ++GfxDeviceGlobal::drawCalls;
}

void ae3d::GfxDevice::ClearScreen( unsigned clearFlags )
{
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc )
{
    System::Assert( startIndex > -1 && startIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in startIndex" );
    System::Assert( endIndex > -1 && endIndex >= startIndex && endIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in endIndex" );
    System::Assert( GfxDeviceGlobal::currentBuffer < GfxDeviceGlobal::drawCmdBuffers.size(), "invalid draw buffer index" );
    System::Assert( GfxDeviceGlobal::pipelineLayout != VK_NULL_HANDLE, "invalid pipelineLayout" );

    const unsigned psoHash = GetPSOHash( vertexBuffer, shader, blendMode, depthFunc );

    if (GfxDeviceGlobal::psoCache.find( psoHash ) == std::end( GfxDeviceGlobal::psoCache ))
    {
        CreatePSO( vertexBuffer, shader, blendMode, depthFunc );
    }

    VkImageView view = VK_NULL_HANDLE;

    // TODO: polymorphism
    if (GfxDeviceGlobal::texture2d0)
    {
        view = GfxDeviceGlobal::texture2d0->GetView();
    }
    else if (GfxDeviceGlobal::textureCube0)
    {
        view = GfxDeviceGlobal::textureCube0->GetView();
    }

    VkDescriptorSet descriptorSet = AllocateDescriptorSet( shader.GetUBODesc(), view );
    GfxDeviceGlobal::pendingFreeDescriptorSets.push_back( descriptorSet );

    vkCmdBindDescriptorSets( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], VK_PIPELINE_BIND_POINT_GRAPHICS,
                             GfxDeviceGlobal::pipelineLayout, 0, 1, &descriptorSet, 0, nullptr );

    vkCmdBindPipeline( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], VK_PIPELINE_BIND_POINT_GRAPHICS, GfxDeviceGlobal::psoCache[ psoHash ] );

    VkDeviceSize offsets[ 1 ] = { 0 };
    vkCmdBindVertexBuffers( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], VertexBuffer::VERTEX_BUFFER_BIND_ID, 1, vertexBuffer.GetVertexBuffer(), offsets );

    vkCmdBindIndexBuffer( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], *vertexBuffer.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16 );
    vkCmdDrawIndexed( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ], (endIndex - startIndex) * 3, 1, startIndex * 3, 0, 0 );
    IncDrawCalls();
}

void ae3d::GfxDevice::ErrorCheck( const char* info )
{
}

void ae3d::GfxDevice::BeginFrame()
{
    VkResult err = acquireNextImageKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, UINT64_MAX, GfxDeviceGlobal::presentCompleteSemaphore, (VkFence)nullptr, &GfxDeviceGlobal::currentBuffer );
    CheckVulkanResult( err, "acquireNextImage" );

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
    CheckVulkanResult( err, "vkQueueSubmit" );

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
    CheckVulkanResult( err, "queuePresent" );

    err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
    CheckVulkanResult( err, "vkQueueWaitIdle" );

    for (std::size_t i = 0; i < GfxDeviceGlobal::pendingFreeDescriptorSets.size(); ++i)
    {
        vkFreeDescriptorSets( GfxDeviceGlobal::device, GfxDeviceGlobal::descriptorPool, 1, &GfxDeviceGlobal::pendingFreeDescriptorSets[ i ] );
    }

    GfxDeviceGlobal::pendingFreeDescriptorSets.clear();

    for (std::size_t i = 0; i < GfxDeviceGlobal::pendingFreeVBs.size(); ++i)
    {
        vkDestroyBuffer( GfxDeviceGlobal::device, GfxDeviceGlobal::pendingFreeVBs[ i ], nullptr );
    }

    GfxDeviceGlobal::pendingFreeVBs.clear();
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    vkDeviceWaitIdle( GfxDeviceGlobal::device );

    debug::Free( GfxDeviceGlobal::instance );
    
    for (std::size_t i = 0; i < GfxDeviceGlobal::swapchainBuffers.size(); ++i)
    {
        vkDestroyImageView( GfxDeviceGlobal::device, GfxDeviceGlobal::swapchainBuffers[ i ].view, nullptr );
    }
    
    vkDestroySemaphore( GfxDeviceGlobal::device, GfxDeviceGlobal::renderCompleteSemaphore, nullptr );
    vkDestroySemaphore( GfxDeviceGlobal::device, GfxDeviceGlobal::presentCompleteSemaphore, nullptr );
    vkDestroyPipelineLayout( GfxDeviceGlobal::device, GfxDeviceGlobal::pipelineLayout, nullptr );
    vkDestroySwapchainKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, nullptr );
    vkDestroyCommandPool( GfxDeviceGlobal::device, GfxDeviceGlobal::cmdPool, nullptr );
    vkDestroyDevice( GfxDeviceGlobal::device, nullptr );
    vkDestroyInstance( GfxDeviceGlobal::instance, nullptr );
}

void ae3d::GfxDevice::SetRenderTarget( RenderTexture* /*target*/, unsigned /*cubeMapFace*/ )
{
}

void ae3d::GfxDevice::SetMultiSampling( bool /*enable*/ )
{
}