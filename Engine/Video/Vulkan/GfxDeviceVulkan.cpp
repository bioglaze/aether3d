#include "GfxDevice.hpp"
#include <cstdint>
#include <cstring>
#include <map>
#include <vector> 
#include <string>
#include <vulkan/vulkan.h>
#include "FileSystem.hpp"
#include "LightTiler.hpp"
#include "Macros.hpp"
#include "RenderTexture.hpp"
#include "Renderer.hpp"
#include "System.hpp"
#include "Shader.hpp"
#include "Statistics.hpp"
#include "VertexBuffer.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "VulkanUtils.hpp"
#if VK_USE_PLATFORM_XCB_KHR
#include <X11/Xlib-xcb.h>
#endif

extern ae3d::Renderer renderer;
void EndOffscreen();

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
    VkDeviceMemory uboMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo uboDesc = {};
    std::uint8_t* uboData = nullptr;
};

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
    VkPhysicalDeviceProperties properties;
    VkClearColorValue clearColor;
    
    std::vector< VkCommandBuffer > drawCmdBuffers;
    VkCommandBuffer setupCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer prePresentCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer postPresentCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer computeCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer offscreenCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer currentCmdBuffer = VK_NULL_HANDLE;
    VkCommandBuffer texCmdBuffer = VK_NULL_HANDLE;
    
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
    std::uint32_t graphicsQueueIndex = 0;
    std::vector< VkImage > swapchainImages;
    std::vector< SwapchainBuffer > swapchainBuffers;
    std::vector< VkFramebuffer > frameBuffers;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkSemaphore presentCompleteSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
    VkSemaphore offscreenSemaphore = VK_NULL_HANDLE;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkQueryPool queryPool = VK_NULL_HANDLE;
    std::vector< float > timings;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::map< std::uint64_t, VkPipeline > psoCache;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector< VkDescriptorSet > descriptorSets;
    int descriptorSetIndex = 0;
    std::uint32_t queueNodeIndex = UINT32_MAX;
    std::uint32_t currentBuffer = 0;
    ae3d::RenderTexture* renderTexture0 = nullptr;
    VkFramebuffer frameBuffer0 = VK_NULL_HANDLE;
    VkImageView view0 = VK_NULL_HANDLE;
    VkImageView view1 = VK_NULL_HANDLE;
    VkSampler sampler0 = VK_NULL_HANDLE;
    VkSampler sampler1 = VK_NULL_HANDLE;
    std::vector< VkBuffer > pendingFreeVBs;
    std::vector< Ubo > ubos;
    int currentUbo = 0;
    VkSampleCountFlagBits msaaSampleBits = VK_SAMPLE_COUNT_1_BIT;
    int backBufferWidth;
    int backBufferHeight;
    unsigned frameIndex = 0;
    ae3d::LightTiler lightTiler;
    PerObjectUboStruct perObjectUboStruct;
    ae3d::VertexBuffer uiVertexBuffer;
    std::vector< ae3d::VertexBuffer::VertexPTC > uiVertices( 512 * 1024 );
    std::vector< ae3d::VertexBuffer::Face > uiFaces( 512 * 1024 );
    std::vector< ae3d::VertexBuffer > lineBuffers;
    bool usedOffscreen = false;
}

namespace ae3d
{
    namespace System
    {
        namespace Statistics
        {
            std::string GetStatistics()
            {
                /*std::stringstream stm;
                stm << "frame time: " << ::Statistics::GetFrameTimeMS() << " ms\n";
                stm << "shadow pass time CPU: " << ::Statistics::GetShadowMapTimeMS() << " ms\n";
                stm << "shadow pass time GPU: " << ::Statistics::GetShadowMapTimeGpuMS() << " ms\n";
                stm << "depth pass time CPU: " << ::Statistics::GetDepthNormalsTimeMS() << " ms\n";
                stm << "depth pass time GPU: " << ::Statistics::GetDepthNormalsTimeGpuMS() << " ms\n";
                stm << "draw calls: " << ::Statistics::GetDrawCalls() << "\n";
                stm << "barrier calls: " << ::Statistics::GetBarrierCalls() << "\n";
                stm << "fence calls: " << ::Statistics::GetFenceCalls() << "\n";
                stm << "mem alloc calls: " << ::Statistics::GetAllocCalls() << " (frame), " << ::Statistics::GetTotalAllocCalls() << " (total)\n";
                stm << "triangles: " << ::Statistics::GetTriangleCount() << "\n";*/

                std::string str;
                str = "frame time: " + std::to_string( ::Statistics::GetFrameTimeMS() ) + " ms\n";
                str += "shadow pass time CPU: " + std::to_string( ::Statistics::GetShadowMapTimeMS() ) + " ms\n";
                str += "shadow pass time GPU: " + std::to_string( ::Statistics::GetShadowMapTimeGpuMS() ) + " ms\n";
                str += "depth pass time CPU: " + std::to_string( ::Statistics::GetDepthNormalsTimeMS() ) + " ms\n";
                str += "depth pass time GPU: " + std::to_string( ::Statistics::GetDepthNormalsTimeGpuMS() ) + " ms\n";
                str += "draw calls: " + std::to_string( ::Statistics::GetDrawCalls() ) + "\n";
                str += "barrier calls: " + std::to_string( ::Statistics::GetBarrierCalls() ) + "\n";
                str += "fence calls: " + std::to_string( ::Statistics::GetFenceCalls() ) + "\n";
                str += "mem alloc calls: " + std::to_string( ::Statistics::GetAllocCalls() ) + " (frame), " + std::to_string( ::Statistics::GetTotalAllocCalls() ) + " (total)\n";
                str += "triangles: " + std::to_string( ::Statistics::GetTriangleCount() ) + "\n";

                return str;
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
    extern int presentInterval;
}

namespace ae3d
{
    std::uint32_t GetMemoryType( std::uint32_t typeBits, VkFlags properties )
    {
        for (std::uint32_t i = 0; i < 32; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((GfxDeviceGlobal::deviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }
            typeBits >>= 1;
        }

        ae3d::System::Assert( false, "could not get memory type" );
        return 0;
    }

    void CreateMsaaColor()
    {
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
        memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        
        err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &GfxDeviceGlobal::msaaTarget.colorMem );
        Statistics::IncTotalAllocCalls();
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
        memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

        err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &GfxDeviceGlobal::msaaTarget.depthMem );
        Statistics::IncTotalAllocCalls();
        AE3D_CHECK_VULKAN( err, "MSAA depth memory" );
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
        AE3D_CHECK_VULKAN( err, "MSAA depth view" );
    }

    void CreatePSO( VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode, ae3d::GfxDevice::DepthFunc depthFunc,
                    ae3d::GfxDevice::CullMode cullMode, ae3d::GfxDevice::FillMode fillMode, VkRenderPass renderPass, ae3d::GfxDevice::PrimitiveTopology topology, std::uint64_t hash )
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
        inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.topology = topology == ae3d::GfxDevice::PrimitiveTopology::Triangles ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterizationState = {};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.polygonMode = fillMode == ae3d::GfxDevice::FillMode::Solid ? VK_POLYGON_MODE_FILL : VK_POLYGON_MODE_LINE;
        
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

        VkPipelineShaderStageCreateInfo shaderStages[ 2 ] = { shader.GetVertexInfo(), shader.GetFragmentInfo() };

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};

        pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCreateInfo.layout = GfxDeviceGlobal::pipelineLayout;
        pipelineCreateInfo.renderPass = renderPass != VK_NULL_HANDLE ? renderPass : GfxDeviceGlobal::renderPass;
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

        for (std::size_t i = 0; i < GfxDeviceGlobal::drawCmdBuffers.size(); ++i)
        {
            debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::drawCmdBuffers[ i ], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "drawCmdBuffer" );
        }
        
        commandBufferAllocateInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::postPresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::postPresentCmdBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "postPresentCmdBuffer" );

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::prePresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::prePresentCmdBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "prePresentCmdBuffer" );

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::offscreenCmdBuffer );
        AE3D_CHECK_VULKAN( err, "Offscreen command buffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::offscreenCmdBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "offscreenCmdBuffer" );

        VkCommandBufferAllocateInfo computeBufAllocateInfo = {};
        computeBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        computeBufAllocateInfo.commandPool = GfxDeviceGlobal::cmdPool;
        computeBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        computeBufAllocateInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &computeBufAllocateInfo, &GfxDeviceGlobal::computeCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::computeCmdBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, "computeCmdBuffer" );
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
        Statistics::IncBarrierCalls();

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
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &postPresentBarrier );
        Statistics::IncBarrierCalls();

        err = vkEndCommandBuffer( GfxDeviceGlobal::postPresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );

        VkSubmitInfo submitPostInfo = {};
        submitPostInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitPostInfo.commandBufferCount = 1;
        submitPostInfo.pCommandBuffers = &GfxDeviceGlobal::postPresentCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitPostInfo, VK_NULL_HANDLE );
        AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );

        Statistics::EndFrameTimeProfiling();
    }
    
    void AllocateSetupCommandBuffer()
    {
        System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized." );

        if (GfxDeviceGlobal::setupCmdBuffer != VK_NULL_HANDLE)
        {
            vkFreeCommandBuffers( GfxDeviceGlobal::device, GfxDeviceGlobal::cmdPool, 1, &GfxDeviceGlobal::setupCmdBuffer );
            GfxDeviceGlobal::setupCmdBuffer = VK_NULL_HANDLE;
        }

        if (GfxDeviceGlobal::cmdPool == VK_NULL_HANDLE)
        {
            VkCommandPoolCreateInfo cmdPoolInfo = {};
            cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmdPoolInfo.queueFamilyIndex = GfxDeviceGlobal::queueNodeIndex;
            cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            VkResult err = vkCreateCommandPool( GfxDeviceGlobal::device, &cmdPoolInfo, nullptr, &GfxDeviceGlobal::cmdPool );
            AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
        }

        VkCommandBufferAllocateInfo info = {};
        info.commandBufferCount = 1;
        info.commandPool = GfxDeviceGlobal::cmdPool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.pNext = nullptr;
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

        VkResult err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &info, &GfxDeviceGlobal::setupCmdBuffer );
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
        if (surfCaps.currentExtent.width == (std::uint32_t)-1)
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

        GfxDeviceGlobal::backBufferWidth = WindowGlobal::windowWidth;
        GfxDeviceGlobal::backBufferHeight = WindowGlobal::windowHeight;

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
        swapchainInfo.presentMode = WindowGlobal::presentInterval == 0 ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR;
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

        vkGetPhysicalDeviceProperties( GfxDeviceGlobal::physicalDevice, &GfxDeviceGlobal::properties );
        
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
        GfxDeviceGlobal::graphicsQueueIndex = graphicsQueueIndex;

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

        vkGetPhysicalDeviceFeatures( GfxDeviceGlobal::physicalDevice, &GfxDeviceGlobal::deviceFeatures );

        VkPhysicalDeviceFeatures enabledFeatures = {};
        enabledFeatures.tessellationShader = GfxDeviceGlobal::deviceFeatures.tessellationShader;
        enabledFeatures.shaderTessellationAndGeometryPointSize = GfxDeviceGlobal::deviceFeatures.shaderTessellationAndGeometryPointSize;
        enabledFeatures.shaderClipDistance = GfxDeviceGlobal::deviceFeatures.shaderClipDistance;
        enabledFeatures.shaderCullDistance = GfxDeviceGlobal::deviceFeatures.shaderCullDistance;
        enabledFeatures.textureCompressionBC = GfxDeviceGlobal::deviceFeatures.textureCompressionBC;
        enabledFeatures.fillModeNonSolid = GfxDeviceGlobal::deviceFeatures.fillModeNonSolid;
        enabledFeatures.samplerAnisotropy = GfxDeviceGlobal::deviceFeatures.samplerAnisotropy;

        if (debug::enabled)
        {
            enabledFeatures.robustBufferAccess = GfxDeviceGlobal::deviceFeatures.robustBufferAccess;
        }
        
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

        const std::vector< VkFormat > depthFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
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
        attachments[ 3 ].samples = GfxDeviceGlobal::msaaSampleBits;
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
        mem_alloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &mem_alloc, nullptr, &GfxDeviceGlobal::depthStencil.mem );
        Statistics::IncTotalAllocCalls();
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
        const int AE3D_DESCRIPTOR_SETS_COUNT = 1550;

        const std::uint32_t typeCount = 11;
        VkDescriptorPoolSize typeCounts[ typeCount ];
        typeCounts[ 0 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        typeCounts[ 0 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 1 ].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        typeCounts[ 1 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 2 ].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        typeCounts[ 2 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 3 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        typeCounts[ 3 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 4 ].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        typeCounts[ 4 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 5 ].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        typeCounts[ 5 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 6 ].type = VK_DESCRIPTOR_TYPE_SAMPLER;
        typeCounts[ 6 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 7 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        typeCounts[ 7 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 8 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        typeCounts[ 8 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
        typeCounts[ 9 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        typeCounts[ 9 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;
		typeCounts[10 ].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		typeCounts[10 ].descriptorCount = AE3D_DESCRIPTOR_SETS_COUNT;

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.pNext = nullptr;
        descriptorPoolInfo.poolSizeCount = typeCount;
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

    VkDescriptorSet AllocateDescriptorSet( const VkDescriptorBufferInfo& uboDesc, const VkImageView& view0, VkSampler sampler0, const VkImageView& view1, VkSampler sampler1 )
    {
        VkDescriptorSet outDescriptorSet = GfxDeviceGlobal::descriptorSets[ GfxDeviceGlobal::descriptorSetIndex ];
        GfxDeviceGlobal::descriptorSetIndex = (GfxDeviceGlobal::descriptorSetIndex + 1) % GfxDeviceGlobal::descriptorSets.size();

        // Binding 0 : Uniform buffer
        VkWriteDescriptorSet uboSet = {};
        uboSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uboSet.dstSet = outDescriptorSet;
        uboSet.descriptorCount = 1;
        uboSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboSet.pBufferInfo = &uboDesc;
        uboSet.dstBinding = 0;

        VkDescriptorImageInfo sampler0Desc = {};
        sampler0Desc.sampler = sampler0;
        sampler0Desc.imageView = view0;
        sampler0Desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Binding 1 : Image
        VkWriteDescriptorSet imageSet = {};
        imageSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageSet.dstSet = outDescriptorSet;
        imageSet.descriptorCount = 1;
        imageSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        imageSet.pImageInfo = &sampler0Desc;
        imageSet.dstBinding = 1;

        VkDescriptorImageInfo sampler1Desc = {};
        sampler1Desc.sampler = sampler1;
        sampler1Desc.imageView = view1;
        sampler1Desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Binding 2 : Sampler
        VkWriteDescriptorSet samplerSet = {};
        samplerSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerSet.dstSet = outDescriptorSet;
        samplerSet.descriptorCount = 1;
        samplerSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerSet.pImageInfo = &sampler0Desc;
        samplerSet.dstBinding = 2;

        // Binding 3 : Buffer
        VkWriteDescriptorSet bufferSet = {};
        bufferSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        bufferSet.dstSet = outDescriptorSet;
        bufferSet.descriptorCount = 1;
        bufferSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        bufferSet.pTexelBufferView = GfxDeviceGlobal::lightTiler.GetPointLightBufferView();
        bufferSet.dstBinding = 3;

        // Binding 4 : Buffer (UAV)
        VkWriteDescriptorSet bufferSetUAV = {};
        bufferSetUAV.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        bufferSetUAV.dstSet = outDescriptorSet;
        bufferSetUAV.descriptorCount = 1;
        bufferSetUAV.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        bufferSetUAV.pTexelBufferView = GfxDeviceGlobal::lightTiler.GetLightIndexBufferView();
        bufferSetUAV.dstBinding = 4;

        // Binding 5 : Image
        VkWriteDescriptorSet imageSet2 = {};
        imageSet2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        imageSet2.dstSet = outDescriptorSet;
        imageSet2.descriptorCount = 1;
        imageSet2.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        imageSet2.pImageInfo = &sampler1Desc;
        imageSet2.dstBinding = 5;

        // Binding 6 : Sampler
        VkWriteDescriptorSet samplerSet2 = {};
        samplerSet2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerSet2.dstSet = outDescriptorSet;
        samplerSet2.descriptorCount = 1;
        samplerSet2.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        samplerSet2.pImageInfo = &sampler1Desc;
        samplerSet2.dstBinding = 6;

        // Binding 7 : Buffer
        VkWriteDescriptorSet bufferSet2 = {};
        bufferSet2.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        bufferSet2.dstSet = outDescriptorSet;
        bufferSet2.descriptorCount = 1;
        bufferSet2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        bufferSet2.pTexelBufferView = GfxDeviceGlobal::lightTiler.GetPointLightColorBufferView();
        bufferSet2.dstBinding = 7;

        // Binding 8 : Buffer
        VkWriteDescriptorSet bufferSet3 = {};
        bufferSet3.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        bufferSet3.dstSet = outDescriptorSet;
        bufferSet3.descriptorCount = 1;
        bufferSet3.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        bufferSet3.pTexelBufferView = GfxDeviceGlobal::lightTiler.GetSpotLightBufferView();
        bufferSet3.dstBinding = 8;

        // Binding 9 : Buffer
        VkWriteDescriptorSet bufferSet4 = {};
        bufferSet4.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        bufferSet4.dstSet = outDescriptorSet;
        bufferSet4.descriptorCount = 1;
        bufferSet4.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        bufferSet4.pTexelBufferView = GfxDeviceGlobal::lightTiler.GetSpotLightParamsView();
        bufferSet4.dstBinding = 9;

		// Binding 10 : Buffer
		VkWriteDescriptorSet bufferSet5 = {};
		bufferSet5.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		bufferSet5.dstSet = outDescriptorSet;
		bufferSet5.descriptorCount = 1;
		bufferSet5.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		bufferSet5.pTexelBufferView = GfxDeviceGlobal::lightTiler.GetSpotLightColorBufferView();
		bufferSet5.dstBinding = 10;

        const int setCount = 11;
        VkWriteDescriptorSet sets[ setCount ] = { uboSet, samplerSet, imageSet, bufferSet, bufferSetUAV, imageSet2, samplerSet2, bufferSet2, bufferSet3, bufferSet4, bufferSet5 };
        vkUpdateDescriptorSets( GfxDeviceGlobal::device, setCount, sets, 0, nullptr );

        return outDescriptorSet;
    }

    void CreateDescriptorSetLayout()
    {
        // Binding 0 : Uniform buffer
        VkDescriptorSetLayoutBinding layoutBindingUBO = {};
        layoutBindingUBO.binding = 0;
        layoutBindingUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBindingUBO.descriptorCount = 1;
        layoutBindingUBO.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingUBO.pImmutableSamplers = nullptr;

        // Binding 1 : Image
        VkDescriptorSetLayoutBinding layoutBindingImage = {};
        layoutBindingImage.binding = 1;
        layoutBindingImage.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        layoutBindingImage.descriptorCount = 1;
        layoutBindingImage.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingImage.pImmutableSamplers = nullptr;

        // Binding 2 : Sampler
        VkDescriptorSetLayoutBinding layoutBindingSampler = {};
        layoutBindingSampler.binding = 2;
        layoutBindingSampler.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        layoutBindingSampler.descriptorCount = 1;
        layoutBindingSampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingSampler.pImmutableSamplers = nullptr;

        // Binding 3 : Buffer
        VkDescriptorSetLayoutBinding layoutBindingBuffer = {};
        layoutBindingBuffer.binding = 3;
        layoutBindingBuffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        layoutBindingBuffer.descriptorCount = 1;
        layoutBindingBuffer.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingBuffer.pImmutableSamplers = nullptr;

        // Binding 4 : Buffer (UAV)
        VkDescriptorSetLayoutBinding layoutBindingBufferUAV = {};
        layoutBindingBufferUAV.binding = 4;
        layoutBindingBufferUAV.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        layoutBindingBufferUAV.descriptorCount = 1;
        layoutBindingBufferUAV.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingBufferUAV.pImmutableSamplers = nullptr;

        // Binding 5 : Image
        VkDescriptorSetLayoutBinding layoutBindingImage2 = {};
        layoutBindingImage2.binding = 5;
        layoutBindingImage2.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        layoutBindingImage2.descriptorCount = 1;
        layoutBindingImage2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingImage2.pImmutableSamplers = nullptr;

        // Binding 6 : Sampler
        VkDescriptorSetLayoutBinding layoutBindingSampler2 = {};
        layoutBindingSampler2.binding = 6;
        layoutBindingSampler2.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        layoutBindingSampler2.descriptorCount = 1;
        layoutBindingSampler2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingSampler2.pImmutableSamplers = nullptr;

        // Binding 7 : Buffer
        VkDescriptorSetLayoutBinding layoutBindingBuffer2 = {};
        layoutBindingBuffer2.binding = 7;
        layoutBindingBuffer2.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        layoutBindingBuffer2.descriptorCount = 1;
        layoutBindingBuffer2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindingBuffer2.pImmutableSamplers = nullptr;

        // Binding 8 : Buffer
        VkDescriptorSetLayoutBinding layoutBindingBuffer3 = {};
        layoutBindingBuffer3.binding = 8;
        layoutBindingBuffer3.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        layoutBindingBuffer3.descriptorCount = 1;
        layoutBindingBuffer3.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingBuffer3.pImmutableSamplers = nullptr;

        // Binding 9 : Buffer
        VkDescriptorSetLayoutBinding layoutBindingBuffer4 = {};
        layoutBindingBuffer4.binding = 9;
        layoutBindingBuffer4.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        layoutBindingBuffer4.descriptorCount = 1;
        layoutBindingBuffer4.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindingBuffer4.pImmutableSamplers = nullptr;

		// Binding 10 : Buffer
		VkDescriptorSetLayoutBinding layoutBindingBuffer5 = {};
		layoutBindingBuffer5.binding = 10;
		layoutBindingBuffer5.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		layoutBindingBuffer5.descriptorCount = 1;
		layoutBindingBuffer5.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindingBuffer5.pImmutableSamplers = nullptr;

        constexpr int bindingCount = 11;
        const VkDescriptorSetLayoutBinding bindings[ bindingCount ] = { layoutBindingUBO, layoutBindingImage, layoutBindingSampler, layoutBindingBuffer,
                                                                        layoutBindingBufferUAV, layoutBindingImage2, layoutBindingSampler2, layoutBindingBuffer2,
                                                                        layoutBindingBuffer3, layoutBindingBuffer4, layoutBindingBuffer5 };

        VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
        descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayout.pNext = nullptr;
        descriptorLayout.bindingCount = bindingCount;
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

    void CreateSemaphores()
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreCreateInfo.pNext = nullptr;

        VkResult err = vkCreateSemaphore( GfxDeviceGlobal::device, &semaphoreCreateInfo, nullptr, &GfxDeviceGlobal::presentCompleteSemaphore );
        AE3D_CHECK_VULKAN( err, "vkCreateSemaphore" );

        err = vkCreateSemaphore( GfxDeviceGlobal::device, &semaphoreCreateInfo, nullptr, &GfxDeviceGlobal::renderCompleteSemaphore );
        AE3D_CHECK_VULKAN( err, "vkCreateSemaphore" );

        err = vkCreateSemaphore( GfxDeviceGlobal::device, &semaphoreCreateInfo, nullptr, &GfxDeviceGlobal::offscreenSemaphore );
        AE3D_CHECK_VULKAN( err, "vkCreateSemaphore" );
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
            CreateFramebufferMSAA();
        }
        else
        {
            CreateRenderPassNonMSAA();
            CreateFramebufferNonMSAA();
        }

        VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
        pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        VkResult err = vkCreatePipelineCache( GfxDeviceGlobal::device, &pipelineCacheCreateInfo, nullptr, &GfxDeviceGlobal::pipelineCache );
        AE3D_CHECK_VULKAN( err, "vkCreatePipelineCache" );

        FlushSetupCommandBuffer();
        CreateDescriptorSetLayout();
        CreateDescriptorPool();
        CreateSemaphores();        

        GfxDevice::SetClearColor( 0, 0, 0 );
        GfxDevice::CreateUniformBuffers();

        GfxDeviceGlobal::timings.resize( 3 );
        VkQueryPoolCreateInfo queryPoolInfo = {};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = (std::uint32_t)GfxDeviceGlobal::timings.size() - 1;

        err = vkCreateQueryPool( GfxDeviceGlobal::device, &queryPoolInfo, nullptr, &GfxDeviceGlobal::queryPool );
        AE3D_CHECK_VULKAN( err, "vkCreateQueryPool" );

        GfxDeviceGlobal::uiVertexBuffer.Generate( GfxDeviceGlobal::uiFaces.data(), int( GfxDeviceGlobal::uiFaces.size() ), GfxDeviceGlobal::uiVertices.data(), int( GfxDeviceGlobal::uiVertices.size() ), VertexBuffer::Storage::CPU );

        renderer.builtinShaders.lightCullShader.LoadSPIRV( FileSystem::FileContents( "LightCuller.spv" ) );
        renderer.builtinShaders.fullscreenTriangleShader.LoadSPIRV( FileSystem::FileContents( "fullscreen_triangle_vert.spv" ), FileSystem::FileContents( "sprite_frag.spv" ) );

        GfxDeviceGlobal::lightTiler.Init();

        VkCommandBufferAllocateInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufInfo.commandPool = GfxDeviceGlobal::cmdPool;
        cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &cmdBufInfo, &GfxDeviceGlobal::texCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
    }
}

void BindComputeDescriptorSet()
{
    VkDescriptorSet descriptorSet = ae3d::AllocateDescriptorSet( GfxDeviceGlobal::ubos[ GfxDeviceGlobal::currentUbo ].uboDesc, GfxDeviceGlobal::view0, GfxDeviceGlobal::sampler0, GfxDeviceGlobal::view1, GfxDeviceGlobal::sampler1 );

    vkCmdBindDescriptorSets( GfxDeviceGlobal::computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                             GfxDeviceGlobal::pipelineLayout, 0, 1, &descriptorSet, 0, nullptr );
}

void UploadPerObjectUbo()
{
    std::memcpy( &ae3d::GfxDevice::GetCurrentUbo()[ 0 ], &GfxDeviceGlobal::perObjectUboStruct, sizeof( GfxDeviceGlobal::perObjectUboStruct ) );
}

void ae3d::GfxDevice::Init( int width, int height )
{
    GfxDeviceGlobal::backBufferWidth = width;
    GfxDeviceGlobal::backBufferHeight = height;
}

void ae3d::GfxDevice::DrawUI( int vpX, int vpY, int vpWidth, int vpHeight, int elemCount, void* offset )
{
    int scissor[ 4 ] = { vpX < 8192 ? std::abs( vpX )  : 8192, vpY < 8192 ? std::abs( vpY ) : 8192, vpWidth < 8192 ? vpWidth : 8192, vpHeight < 8192 ? vpHeight : 8192 };
    SetScissor( scissor );
    Draw( GfxDeviceGlobal::uiVertexBuffer, (int)((size_t)offset), (int)((size_t)offset + elemCount), renderer.builtinShaders.uiShader, BlendMode::AlphaBlend, DepthFunc::NoneWriteOff, CullMode::Off, FillMode::Solid, GfxDevice::PrimitiveTopology::Triangles );
}

void ae3d::GfxDevice::MapUIVertexBuffer( int /*vertexSize*/, int /*indexSize*/, void** outMappedVertices, void** outMappedIndices )
{
    *outMappedVertices = GfxDeviceGlobal::uiVertices.data();
    *outMappedIndices = GfxDeviceGlobal::uiFaces.data();
}

void ae3d::GfxDevice::UnmapUIVertexBuffer()
{
    GfxDeviceGlobal::uiVertexBuffer.Generate( GfxDeviceGlobal::uiFaces.data(), int( GfxDeviceGlobal::uiFaces.size() ), GfxDeviceGlobal::uiVertices.data(), int( GfxDeviceGlobal::uiVertices.size() ), VertexBuffer::Storage::CPU );
}

void ae3d::GfxDevice::BeginDepthNormalsGpuQuery()
{
}

void ae3d::GfxDevice::EndDepthNormalsGpuQuery()
{
}

void ae3d::GfxDevice::BeginShadowMapGpuQuery()
{
}

void ae3d::GfxDevice::EndShadowMapGpuQuery()
{
}

void ae3d::GfxDevice::SetPolygonOffset( bool, float, float )
{
}

void ae3d::GfxDevice::PushGroupMarker( const char* name )
{
    debug::BeginRegion( GfxDeviceGlobal::currentCmdBuffer, name, 0, 1, 0 );
}

void ae3d::GfxDevice::PopGroupMarker()
{
    debug::EndRegion( GfxDeviceGlobal::currentCmdBuffer );
}

void ae3d::GfxDevice::BeginRenderPassAndCommandBuffer()
{
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

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufInfo.pNext = nullptr;

    VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::currentCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

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
    renderPassBeginInfo.framebuffer = GfxDeviceGlobal::frameBuffers[ GfxDeviceGlobal::currentBuffer ];

    vkCmdBeginRenderPass( GfxDeviceGlobal::currentCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

    VkViewport viewport = {};
    viewport.height = (float)height;
    viewport.width = (float)width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport( GfxDeviceGlobal::currentCmdBuffer, 0, 1, &viewport );

    VkRect2D scissor = {};
    scissor.extent.width = width;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor( GfxDeviceGlobal::currentCmdBuffer, 0, 1, &scissor );
}

void ae3d::GfxDevice::EndRenderPassAndCommandBuffer()
{
    vkCmdEndRenderPass( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ] );

    VkResult err = vkEndCommandBuffer( GfxDeviceGlobal::currentCmdBuffer );
    AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );
}

void ae3d::GfxDevice::SetClearColor( float red, float green, float blue )
{
    GfxDeviceGlobal::clearColor.float32[ 0 ] = red;
    GfxDeviceGlobal::clearColor.float32[ 1 ] = green;
    GfxDeviceGlobal::clearColor.float32[ 2 ] = blue;
    GfxDeviceGlobal::clearColor.float32[ 3 ] = 0.0f;
}

void ae3d::GfxDevice::Set_sRGB_Writes( bool /*enable*/ )
{
}

void ae3d::GfxDevice::GetGpuMemoryUsage( unsigned& outUsedMBytes, unsigned& outBudgetMBytes )
{
    outUsedMBytes = 0;
    outBudgetMBytes = 0;
}

void ae3d::GfxDevice::ClearScreen( unsigned /*clearFlags*/ )
{
}

void ae3d::GfxDevice::DrawLines( int handle, Shader& shader )
{
    if (handle < 0)
    {
        return;
    }

    Draw( GfxDeviceGlobal::lineBuffers[ handle ], 0, GfxDeviceGlobal::lineBuffers[ handle ].GetFaceCount() / 3, shader, BlendMode::Off, DepthFunc::NoneWriteOff, CullMode::Off, FillMode::Solid, GfxDevice::PrimitiveTopology::Lines );
}

void ae3d::GfxDevice::SetViewport( int aViewport[ 4 ] )
{
    VkViewport viewport = {};
    viewport.x = (float)aViewport[ 0 ];
    viewport.y = (float)aViewport[ 1 ];
    viewport.width = (float)aViewport[ 2 ];
    viewport.height = (float)aViewport[ 3 ];
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport( GfxDeviceGlobal::currentCmdBuffer, 0, 1, &viewport );    
}

void ae3d::GfxDevice::SetScissor( int aScissor[ 4 ] )
{
    VkRect2D scissor = {};
    scissor.extent.width = (std::uint32_t)aScissor[ 2 ];
    scissor.extent.height = (std::uint32_t)aScissor[ 3 ];
    scissor.offset.x = (std::uint32_t)aScissor[ 0 ];
    scissor.offset.y = (std::uint32_t)aScissor[ 1 ];
    vkCmdSetScissor( GfxDeviceGlobal::currentCmdBuffer, 0, 1, &scissor );
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc,
                            CullMode cullMode, FillMode fillMode, PrimitiveTopology topology )
{
    System::Assert( startIndex > -1 && startIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in startIndex" );
    System::Assert( endIndex > -1 && endIndex >= startIndex && endIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in endIndex" );
    System::Assert( GfxDeviceGlobal::currentBuffer < GfxDeviceGlobal::drawCmdBuffers.size(), "invalid draw buffer index" );

    if (GfxDeviceGlobal::view0 == VK_NULL_HANDLE || GfxDeviceGlobal::sampler0 == VK_NULL_HANDLE)
    {
        return;
    }

    if (shader.GetVertexInfo().module == VK_NULL_HANDLE || shader.GetFragmentInfo().module == VK_NULL_HANDLE)
    {
        return;
    }

    if (Statistics::GetDrawCalls() >= (int)GfxDeviceGlobal::descriptorSets.size())
    {
        System::Print( "Skipping draw because draw call count %d exceeds descriptor set count %ld \n", Statistics::GetDrawCalls(), GfxDeviceGlobal::descriptorSets.size() );
        return;
    }

    const std::uint64_t psoHash = GetPSOHash( vertexBuffer, shader, blendMode, depthFunc, cullMode, fillMode, GfxDeviceGlobal::renderTexture0 ? GfxDeviceGlobal::renderTexture0->GetRenderPass() : VK_NULL_HANDLE, topology );

    if (GfxDeviceGlobal::psoCache.find( psoHash ) == std::end( GfxDeviceGlobal::psoCache ))
    {
        CreatePSO( vertexBuffer, shader, blendMode, depthFunc, cullMode, fillMode, GfxDeviceGlobal::renderTexture0 ? GfxDeviceGlobal::renderTexture0->GetRenderPass() : VK_NULL_HANDLE, topology, psoHash );
    }

    const unsigned activePointLights = GfxDeviceGlobal::lightTiler.GetPointLightCount();
    const unsigned activeSpotLights = GfxDeviceGlobal::lightTiler.GetSpotLightCount();
    const unsigned lightCount = ((activeSpotLights & 0xFFFFu) << 16) | (activePointLights & 0xFFFFu);

    GfxDeviceGlobal::perObjectUboStruct.windowWidth = GfxDeviceGlobal::backBufferWidth;
    GfxDeviceGlobal::perObjectUboStruct.windowHeight = GfxDeviceGlobal::backBufferHeight;
    GfxDeviceGlobal::perObjectUboStruct.numLights = lightCount;
    GfxDeviceGlobal::perObjectUboStruct.maxNumLightsPerTile = GfxDeviceGlobal::lightTiler.GetMaxNumLightsPerTile();

    UploadPerObjectUbo();

    VkDescriptorSet descriptorSet = AllocateDescriptorSet( GfxDeviceGlobal::ubos[ GfxDeviceGlobal::currentUbo ].uboDesc, GfxDeviceGlobal::view0, GfxDeviceGlobal::sampler0, GfxDeviceGlobal::view1, GfxDeviceGlobal::sampler1 );

    vkCmdBindDescriptorSets( GfxDeviceGlobal::currentCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             GfxDeviceGlobal::pipelineLayout, 0, 1, &descriptorSet, 0, nullptr );

    vkCmdBindPipeline( GfxDeviceGlobal::currentCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GfxDeviceGlobal::psoCache[ psoHash ] );

    VkDeviceSize offsets[ 1 ] = { 0 };
    vkCmdBindVertexBuffers( GfxDeviceGlobal::currentCmdBuffer, VertexBuffer::VERTEX_BUFFER_BIND_ID, 1, vertexBuffer.GetVertexBuffer(), offsets );

    vkCmdBindIndexBuffer( GfxDeviceGlobal::currentCmdBuffer, *vertexBuffer.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16 );
    vkCmdDrawIndexed( GfxDeviceGlobal::currentCmdBuffer, (endIndex - startIndex) * 3, 1, startIndex * 3, 0, 0 );
    Statistics::IncTriangleCount( endIndex - startIndex );
    Statistics::IncDrawCalls();
}

void ae3d::GfxDevice::GetNewUniformBuffer()
{
    GfxDeviceGlobal::currentUbo = (GfxDeviceGlobal::currentUbo + 1) % GfxDeviceGlobal::ubos.size();
}

void ae3d::GfxDevice::CreateUniformBuffers()
{
    GfxDeviceGlobal::ubos.resize( 1800 );

    for (std::size_t uboIndex = 0; uboIndex < GfxDeviceGlobal::ubos.size(); ++uboIndex)
    {
        auto& ubo = GfxDeviceGlobal::ubos[ uboIndex ];

        const VkDeviceSize uboSize = 256 * 3 + 80 * 64;
        static_assert( uboSize >= sizeof( PerObjectUboStruct ), "UBO size must be larger than UBO struct" );

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = uboSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &ubo.ubo );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer UBO" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, ubo.ubo, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &allocInfo, nullptr, &ubo.uboMemory );
        AE3D_CHECK_VULKAN( err, "vkAllocateMemory UBO" );
        Statistics::IncTotalAllocCalls();
        Statistics::IncAllocCalls();

        err = vkBindBufferMemory( GfxDeviceGlobal::device, ubo.ubo, ubo.uboMemory, 0 );
        AE3D_CHECK_VULKAN( err, "vkBindBufferMemory UBO" );

        ubo.uboDesc.buffer = ubo.ubo;
        ubo.uboDesc.offset = 0;
        ubo.uboDesc.range = uboSize;

        err = vkMapMemory( GfxDeviceGlobal::device, ubo.uboMemory, 0, uboSize, 0, (void **)&ubo.uboData );
        AE3D_CHECK_VULKAN( err, "vkMapMemory UBO" );
    }
}

std::uint8_t* ae3d::GfxDevice::GetCurrentUbo()
{
    return GfxDeviceGlobal::ubos[ GfxDeviceGlobal::currentUbo ].uboData;
}

void ae3d::GfxDevice::ErrorCheck( const char* /*info*/ )
{
}

void ae3d::GfxDevice::BeginFrame()
{
    ae3d::System::Assert( acquireNextImageKHR != nullptr, "function pointers not loaded" );
    ae3d::System::Assert( GfxDeviceGlobal::swapChain != VK_NULL_HANDLE, "swap chain not initialized" );
    
    VkResult err = acquireNextImageKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, UINT64_MAX, GfxDeviceGlobal::presentCompleteSemaphore, (VkFence)nullptr, &GfxDeviceGlobal::currentBuffer );

    if (err == VK_TIMEOUT)
    {
        System::Print( "acquireNextImageKHR returned VK_TIMEOUT\n" );
    }

    AE3D_CHECK_VULKAN( err, "acquireNextImage" );

    GfxDeviceGlobal::currentCmdBuffer = GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ];

    SubmitPostPresentBarrier();

    GfxDeviceGlobal::view0 = Texture2D::GetDefaultTexture()->GetView();
    GfxDeviceGlobal::sampler0 = Texture2D::GetDefaultTexture()->GetSampler();
    GfxDeviceGlobal::view1 = Texture2D::GetDefaultTexture()->GetView();
    GfxDeviceGlobal::sampler1 = Texture2D::GetDefaultTexture()->GetSampler();
}

void ae3d::GfxDevice::Present()
{
    VkPipelineStageFlags pipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &pipelineStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &GfxDeviceGlobal::presentCompleteSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &GfxDeviceGlobal::renderCompleteSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ];

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

    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        System::Print( "queuePresentKHR returned VK_ERROR_OUT_OF_DATE_KHR\n" );
    }

    AE3D_CHECK_VULKAN( err, "queuePresent" );

    if (GfxDeviceGlobal::usedOffscreen)
    {
        GfxDeviceGlobal::usedOffscreen = false;
        GfxDeviceGlobal::timings.resize( 1 );
        std::uint32_t start = 0;
        std::uint32_t end = 0;
        
		err = vkGetQueryPoolResults( GfxDeviceGlobal::device, GfxDeviceGlobal::queryPool, 0, 1, sizeof( uint32_t ), &start, 0, VK_QUERY_RESULT_WAIT_BIT );
		AE3D_CHECK_VULKAN( err, "vkGetQueryPoolResults" );

        err = vkGetQueryPoolResults( GfxDeviceGlobal::device, GfxDeviceGlobal::queryPool, 1, 1, sizeof( uint32_t ), &end, 0, VK_QUERY_RESULT_WAIT_BIT );
		AE3D_CHECK_VULKAN( err, "vkGetQueryPoolResults" );

        const float factor = 1e6f * GfxDeviceGlobal::properties.limits.timestampPeriod;
        GfxDeviceGlobal::timings[ 0 ] = (float)(end - start) / factor;

        Statistics::SetDepthNormalsGpuTime( GfxDeviceGlobal::timings[ 0 ] );
    }

    // FIXME: This slows down rendering
    err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
    AE3D_CHECK_VULKAN( err, "vkQueueWaitIdle" );

    for (std::size_t i = 0; i < GfxDeviceGlobal::pendingFreeVBs.size(); ++i)
    {
        vkDestroyBuffer( GfxDeviceGlobal::device, GfxDeviceGlobal::pendingFreeVBs[ i ], nullptr );
    }

    GfxDeviceGlobal::pendingFreeVBs.clear();
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
    vkFreeMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.mem, nullptr );

    vkDestroyDescriptorSetLayout( GfxDeviceGlobal::device, GfxDeviceGlobal::descriptorSetLayout, nullptr );
    vkDestroyDescriptorPool( GfxDeviceGlobal::device, GfxDeviceGlobal::descriptorPool, nullptr );
    vkDestroyRenderPass( GfxDeviceGlobal::device, GfxDeviceGlobal::renderPass, nullptr );
    vkDestroyQueryPool( GfxDeviceGlobal::device, GfxDeviceGlobal::queryPool, nullptr );

    if (GfxDeviceGlobal::msaaTarget.colorImage != VK_NULL_HANDLE)
    {
        vkDestroyImage( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.colorImage, nullptr );
        vkDestroyImage( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.depthImage, nullptr );
        vkDestroyImageView( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.colorView, nullptr );
        vkDestroyImageView( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.depthView, nullptr );
        vkFreeMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.depthMem, nullptr );
        vkFreeMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.colorMem, nullptr );
    }

    for (std::size_t i = 0; i < GfxDeviceGlobal::ubos.size(); ++i)
    {
        vkFreeMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::ubos[ i ].uboMemory, nullptr );
        vkDestroyBuffer( GfxDeviceGlobal::device, GfxDeviceGlobal::ubos[ i ].ubo, nullptr );
    }

    Shader::DestroyShaders();
    ComputeShader::DestroyShaders();
    Texture2D::DestroyTextures();
    TextureCube::DestroyTextures();
    RenderTexture::DestroyTextures();
    VertexBuffer::DestroyBuffers();
    GfxDeviceGlobal::lightTiler.DestroyBuffers();

    for (auto pso : GfxDeviceGlobal::psoCache)
    {
        vkDestroyPipeline( GfxDeviceGlobal::device, pso.second, nullptr );
    }

    vkDestroySemaphore( GfxDeviceGlobal::device, GfxDeviceGlobal::renderCompleteSemaphore, nullptr );
    vkDestroySemaphore( GfxDeviceGlobal::device, GfxDeviceGlobal::presentCompleteSemaphore, nullptr );
    vkDestroySemaphore( GfxDeviceGlobal::device, GfxDeviceGlobal::offscreenSemaphore, nullptr );
    vkDestroyPipelineLayout( GfxDeviceGlobal::device, GfxDeviceGlobal::pipelineLayout, nullptr );
    vkDestroyPipelineCache( GfxDeviceGlobal::device, GfxDeviceGlobal::pipelineCache, nullptr );
    vkDestroySwapchainKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, nullptr );
    vkDestroySurfaceKHR( GfxDeviceGlobal::instance, GfxDeviceGlobal::surface, nullptr );
    vkDestroyCommandPool( GfxDeviceGlobal::device, GfxDeviceGlobal::cmdPool, nullptr );
    vkDestroyDevice( GfxDeviceGlobal::device, nullptr );
    vkDestroyInstance( GfxDeviceGlobal::instance, nullptr );
}

void ae3d::GfxDevice::SetRenderTarget( RenderTexture* target, unsigned /*cubeMapFace*/ )
{
    GfxDeviceGlobal::currentCmdBuffer = target ? GfxDeviceGlobal::offscreenCmdBuffer : GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ];
    GfxDeviceGlobal::renderTexture0 = target;
    GfxDeviceGlobal::frameBuffer0 = target ? target->GetFrameBuffer() : VK_NULL_HANDLE;
}

void BeginOffscreen()
{
    ae3d::System::Assert( GfxDeviceGlobal::renderTexture0 != nullptr, "Render texture must be set when beginning offscreen rendering" );
    
    // FIXME: Use fence instead of queue wait.
    vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::offscreenCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

    vkCmdResetQueryPool( GfxDeviceGlobal::offscreenCmdBuffer, GfxDeviceGlobal::queryPool, 0, 2 );
    vkCmdWriteTimestamp( GfxDeviceGlobal::offscreenCmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, GfxDeviceGlobal::queryPool, 0 );

    VkClearValue clearValues[ 2 ];
    clearValues[ 0 ].color = GfxDeviceGlobal::clearColor;
    clearValues[ 1 ].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = GfxDeviceGlobal::renderTexture0->GetRenderPass();
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = GfxDeviceGlobal::renderTexture0->GetWidth();
    renderPassBeginInfo.renderArea.extent.height = GfxDeviceGlobal::renderTexture0->GetHeight();
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = GfxDeviceGlobal::frameBuffer0;

    vkCmdBeginRenderPass( GfxDeviceGlobal::offscreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

    GfxDeviceGlobal::usedOffscreen = true;
}

void EndOffscreen()
{
    vkCmdWriteTimestamp( GfxDeviceGlobal::offscreenCmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, GfxDeviceGlobal::queryPool, 1 );
    vkCmdEndRenderPass( GfxDeviceGlobal::offscreenCmdBuffer );

    VkResult err = vkEndCommandBuffer( GfxDeviceGlobal::offscreenCmdBuffer );
    AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );

    VkPipelineStageFlags pipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &pipelineStages;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;//&GfxDeviceGlobal::offscreenSemaphore;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::offscreenCmdBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );
}

void ae3d::GfxDevice::SetMultiSampling( bool /*enable*/ )
{
}
