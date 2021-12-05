// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include "GfxDevice.hpp"
#include <cstdint>
#include <map>
#include <vector>
#include <cstring>
#include <string>
#include <vulkan/vulkan.h>
#include "Array.hpp"
#include "FileSystem.hpp"
#include "LightTiler.hpp"
#include "Macros.hpp"
#include "RenderTexture.hpp"
#include "Renderer.hpp"
#include "System.hpp"
#include "Shader.hpp"
#include "Statistics.hpp"
#include "Texture2D.hpp"
#include "TextureCube.hpp"
#include "VertexBuffer.hpp"
#include "VulkanUtils.hpp"
#include "VR.hpp"
#if VK_USE_PLATFORM_XCB_KHR
#include <X11/Xlib-xcb.h>
#endif

extern ae3d::Renderer renderer;
#if VK_USE_PLATFORM_ANDROID_KHR
ANativeWindow* nativeWindow;
#endif

PFN_vkCreateSwapchainKHR createSwapchainKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR getPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR getPhysicalDeviceSurfacePresentModesKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR getPhysicalDeviceSurfaceSupportKHR = nullptr;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR getPhysicalDeviceSurfaceFormatsKHR = nullptr;
PFN_vkGetSwapchainImagesKHR getSwapchainImagesKHR = nullptr;
PFN_vkAcquireNextImageKHR acquireNextImageKHR = nullptr;
PFN_vkQueuePresentKHR queuePresentKHR = nullptr;
PFN_vkGetShaderInfoAMD getShaderInfoAMD;

constexpr unsigned UI_VERTICE_COUNT = 512 * 1024;
constexpr unsigned UI_FACE_COUNT = 128 * 1024;
constexpr std::uint32_t descriptorSlotCount = 17;

namespace Texture2DGlobal
{
    extern std::vector< VkSampler > samplersToReleaseAtExit;
}

extern VkBuffer particleBuffer;
extern VkDeviceMemory particleMemory;

extern VkBuffer particleTileBuffer;
extern VkDeviceMemory particleTileMemory;
extern VkBufferView particleTileBufferView;

struct Ubo
{
    VkBuffer ubo = VK_NULL_HANDLE;
    VkDeviceMemory uboMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo uboDesc = {};
    std::uint8_t* uboData = nullptr;
};

namespace ae3d
{
    namespace GfxDevice
    {
        unsigned backBufferWidth;
        unsigned backBufferHeight;
    }
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
    VkPhysicalDeviceProperties properties;
    VkClearColorValue clearColor;
    
    VkCommandBuffer drawCmdBuffers[ 4 ];
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
    Array< SwapchainBuffer > swapchainBuffers;
    Array< VkFramebuffer > frameBuffers;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkSemaphore presentCompleteSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkQueryPool queryPool = VK_NULL_HANDLE;
    float timings[ 3 ];
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    std::map< std::uint64_t, VkPipeline > psoCache;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    Array< VkDescriptorSet > descriptorSets;
    unsigned descriptorSetIndex = 0;
    std::uint32_t queueNodeIndex = UINT32_MAX;
    std::uint32_t currentBuffer = 0;
    ae3d::RenderTexture* renderTexture0 = nullptr;
    VkFramebuffer frameBuffer0 = VK_NULL_HANDLE;
    VkImageView boundViews[ ae3d::ComputeShader::SLOT_COUNT ];
    VkSampler boundSamplers[ 2 ];
    VkSampler linearRepeat;
    Array< VkBuffer > pendingFreeVBs;
    Array< VkDeviceMemory > pendingFreeMemory;
    Array< Ubo > ubos;
    unsigned currentUbo = 0;
    VkSampleCountFlagBits msaaSampleBits = VK_SAMPLE_COUNT_1_BIT;
    ae3d::LightTiler lightTiler;
    PerObjectUboStruct perObjectUboStruct;
    ae3d::VertexBuffer uiVertexBuffer;
    ae3d::VertexBuffer::VertexPTC uiVertices[ UI_VERTICE_COUNT ];
    ae3d::VertexBuffer::Face uiFaces[ UI_FACE_COUNT ];
    std::vector< ae3d::VertexBuffer > lineBuffers;
    VkPipeline cachedPSO;
}

namespace ae3d
{
    namespace System
    {
        namespace Statistics
        {
            void GetStatistics( char* outStr )
            {
                std::string str;
                str = "frame time: " + std::to_string( ::Statistics::GetFrameTimeMS() ) + " ms\n";
                str += "present time CPU: " + std::to_string( ::Statistics::GetPresentTimeMS() ) + " ms\n";                
                str += "shadow pass time CPU: " + std::to_string( ::Statistics::GetShadowMapTimeMS() ) + " ms\n";
                str += "shadow pass time GPU: " + std::to_string( ::Statistics::GetShadowMapTimeGpuMS() ) + " ms\n";
                str += "depth pass time CPU: " + std::to_string( ::Statistics::GetDepthNormalsTimeMS() ) + " ms\n";
                str += "depth pass time GPU: " + std::to_string( ::Statistics::GetDepthNormalsTimeGpuMS() ) + " ms\n";
                str += "primary pass time GPU: " + std::to_string( ::Statistics::GetPrimaryPassTimeGpuMS() ) + " ms\n";
                str += "bloom CPU: " + std::to_string( ::Statistics::GetBloomCpuTimeMS() ) + " ms\n";
                //str += "bloom GPU: " + std::to_string( ::Statistics::GetBloomGpuTimeMS() ) + " ms\n";
                str += "queue wait: " + std::to_string( ::Statistics::GetQueueWaitTimeMS() ) + " ms \n";
                str += "frustum cull: " + std::to_string( ::Statistics::GetFrustumCullTimeMS() ) + " ms \n";
                str += "draw calls: " + std::to_string( ::Statistics::GetDrawCalls() ) + "\n";
                str += "barrier calls: " + std::to_string( ::Statistics::GetBarrierCalls() ) + "\n";
				str += "fence calls: " + std::to_string( ::Statistics::GetFenceCalls() ) + "\n";
				str += "pso changes: " + std::to_string( ::Statistics::GetPSOBindCalls() ) + "\n";
                str += "queue submit calls: " + std::to_string( ::Statistics::GetQueueSubmitCalls() ) + "\n";
                str += "mem alloc calls: " + std::to_string( ::Statistics::GetAllocCalls() ) + " (frame), " + std::to_string( ::Statistics::GetTotalAllocCalls() ) + " (total)\n";
                str += "triangles: " + std::to_string( ::Statistics::GetTriangleCount() ) + "\n";

				std::strncpy( outStr, str.c_str(), 512 );
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
        for (std::uint32_t i = 0; i < 32; ++i)
        {
            if ((typeBits & 1) == 1 && (GfxDeviceGlobal::deviceMemoryProperties.memoryTypes[ i ].propertyFlags & properties) == properties )
            {
                return i;
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
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::msaaTarget.colorImage, VK_OBJECT_TYPE_IMAGE, "MSAA color" );

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.colorImage, &memReqs );
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        
        err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &GfxDeviceGlobal::msaaTarget.colorMem );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::msaaTarget.colorMem, VK_OBJECT_TYPE_DEVICE_MEMORY, "msaaTarget colorMemory" );

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
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::msaaTarget.depthImage, VK_OBJECT_TYPE_IMAGE, "MSAA depth" );

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements( GfxDeviceGlobal::device, GfxDeviceGlobal::msaaTarget.depthImage, &memReqs );
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

        err = vkAllocateMemory( GfxDeviceGlobal::device, &memAlloc, nullptr, &GfxDeviceGlobal::msaaTarget.depthMem );
        Statistics::IncTotalAllocCalls();
        AE3D_CHECK_VULKAN( err, "MSAA depth memory" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::msaaTarget.depthMem, VK_OBJECT_TYPE_DEVICE_MEMORY, "msaaTarget depthMemory" );

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
            depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
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
        
        if (GfxDeviceGlobal::renderTexture0 && GfxDeviceGlobal::renderTexture0->GetSampleCount() == 1)
        {
            multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        }

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
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)pso, VK_OBJECT_TYPE_PIPELINE, "graphics pipeline" );

        GfxDeviceGlobal::psoCache[ hash ] = pso;
    }

    void AllocateCommandBuffers()
    {
        System::Assert( GfxDeviceGlobal::cmdPool != VK_NULL_HANDLE, "command pool not initialized" );
        System::Assert( GfxDeviceGlobal::device != VK_NULL_HANDLE, "device not initialized" );
        System::Assert( GfxDeviceGlobal::swapchainBuffers.count > 0, "imageCount is 0" );

        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = GfxDeviceGlobal::cmdPool;
        commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferAllocateInfo.commandBufferCount = GfxDeviceGlobal::swapchainBuffers.count;

        VkResult err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, GfxDeviceGlobal::drawCmdBuffers );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );

        for (unsigned i = 0; i < GfxDeviceGlobal::swapchainBuffers.count; ++i)
        {
            debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::drawCmdBuffers[ i ], VK_OBJECT_TYPE_COMMAND_BUFFER, "drawCmdBuffer" );
        }
        
        commandBufferAllocateInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::postPresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::postPresentCmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "postPresentCmdBuffer" );

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::prePresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::prePresentCmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "prePresentCmdBuffer" );

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &commandBufferAllocateInfo, &GfxDeviceGlobal::offscreenCmdBuffer );
        AE3D_CHECK_VULKAN( err, "Offscreen command buffer" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::offscreenCmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "offscreenCmdBuffer" );

        VkCommandBufferAllocateInfo computeBufAllocateInfo = {};
        computeBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        computeBufAllocateInfo.commandPool = GfxDeviceGlobal::cmdPool;
        computeBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        computeBufAllocateInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &computeBufAllocateInfo, &GfxDeviceGlobal::computeCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::computeCmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "computeCmdBuffer" );
    }

    void SubmitPrePresentBarrier()
    {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::prePresentCmdBuffer, &cmdBufInfo );
        AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

        SetImageLayout( GfxDeviceGlobal::prePresentCmdBuffer, GfxDeviceGlobal::swapchainBuffers[ GfxDeviceGlobal::currentBuffer ].image, VK_IMAGE_ASPECT_COLOR_BIT,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, 0, 1 );

        err = vkEndCommandBuffer( GfxDeviceGlobal::prePresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &GfxDeviceGlobal::prePresentCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
        AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );
        Statistics::IncQueueSubmitCalls();
    }

    void SubmitPostPresentBarrier()
    {
        VkCommandBufferBeginInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::postPresentCmdBuffer, &cmdBufInfo );
        AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

        SetImageLayout( GfxDeviceGlobal::postPresentCmdBuffer, GfxDeviceGlobal::swapchainBuffers[ GfxDeviceGlobal::currentBuffer ].image, VK_IMAGE_ASPECT_COLOR_BIT,
                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 0, 1 );

        err = vkEndCommandBuffer( GfxDeviceGlobal::postPresentCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );

        VkSubmitInfo submitPostInfo = {};
        submitPostInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitPostInfo.commandBufferCount = 1;
        submitPostInfo.pCommandBuffers = &GfxDeviceGlobal::postPresentCmdBuffer;

        err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitPostInfo, VK_NULL_HANDLE );
        AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );
        Statistics::IncQueueSubmitCalls();

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
#if VK_USE_PLATFORM_ANDROID_KHR
        VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.window = nativeWindow;
        err = vkCreateAndroidSurfaceKHR( GfxDeviceGlobal::instance, &surfaceCreateInfo, nullptr, &GfxDeviceGlobal::surface );
#endif
        AE3D_CHECK_VULKAN( err, "create surface" );
        System::Assert( GfxDeviceGlobal::surface != VK_NULL_HANDLE, "no surface" );
        
        std::uint32_t queueCount;
        vkGetPhysicalDeviceQueueFamilyProperties( GfxDeviceGlobal::physicalDevice, &queueCount, nullptr );
        System::Assert( queueCount >= 1 && queueCount < 5, "None or more queues than buffers have elements! Increase element count." );

        VkQueueFamilyProperties queueProps[ 5 ];
        vkGetPhysicalDeviceQueueFamilyProperties( GfxDeviceGlobal::physicalDevice, &queueCount, &queueProps[ 0 ] );

        VkBool32 supportsPresent[ 5 ];

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
        
        Array< VkSurfaceFormatKHR > surfFormats;
        surfFormats.Allocate( formatCount );
        
        err = getPhysicalDeviceSurfaceFormatsKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &formatCount, surfFormats.elements );
        AE3D_CHECK_VULKAN( err, "getPhysicalDeviceSurfaceFormatsKHR" );

        bool foundSRGB = false;
        VkFormat sRGBFormat = VK_FORMAT_B8G8R8A8_SRGB;

        for (unsigned formatIndex = 0; formatIndex < surfFormats.count; ++formatIndex)
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
        
        Array< VkPresentModeKHR > presentModes;
        presentModes.Allocate( presentModeCount );
        
        err = getPhysicalDeviceSurfacePresentModesKHR( GfxDeviceGlobal::physicalDevice, GfxDeviceGlobal::surface, &presentModeCount, presentModes.elements );
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

        GfxDevice::backBufferWidth = WindowGlobal::windowWidth;
        GfxDevice::backBufferHeight = WindowGlobal::windowHeight;

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
#if VK_USE_PLATFORM_ANDROID_KHR
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
#else
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
#endif

        err = createSwapchainKHR( GfxDeviceGlobal::device, &swapchainInfo, nullptr, &GfxDeviceGlobal::swapChain );
        AE3D_CHECK_VULKAN( err, "swapchain" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::swapChain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "swap chain" );

        std::uint32_t imageCount;
        err = getSwapchainImagesKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, &imageCount, nullptr );
        AE3D_CHECK_VULKAN( err, "getSwapchainImagesKHR" );
        ae3d::System::Assert( imageCount > 0, "imageCount" );

        Array< VkImage > swapchainImages( imageCount );

        err = getSwapchainImagesKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, &imageCount, swapchainImages.elements );
        AE3D_CHECK_VULKAN( err, "getSwapchainImagesKHR" );

        GfxDeviceGlobal::swapchainBuffers.Allocate( imageCount );

        for (std::uint32_t i = 0; i < imageCount; ++i)
        {
            GfxDeviceGlobal::swapchainBuffers[ i ].image = swapchainImages[ i ];
            debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)swapchainImages[ i ], VK_OBJECT_TYPE_IMAGE, "swapchain image" );

            VkImageViewCreateInfo colorAttachmentView = {};
            colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
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
            colorAttachmentView.image = GfxDeviceGlobal::swapchainBuffers[ i ].image;

            err = vkCreateImageView( GfxDeviceGlobal::device, &colorAttachmentView, nullptr, &GfxDeviceGlobal::swapchainBuffers[ i ].view );
            AE3D_CHECK_VULKAN( err, "vkCreateImageView" );

            SetImageLayout(
            GfxDeviceGlobal::setupCmdBuffer,
            GfxDeviceGlobal::swapchainBuffers[ i ].image,
            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 1, 0, 1 );
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

        getShaderInfoAMD = (PFN_vkGetShaderInfoAMD)vkGetDeviceProcAddr( GfxDeviceGlobal::device, "vkGetShaderInfoAMD" );
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

        Array< VkQueueFamilyProperties > queueProps;
        queueProps.Allocate( queueCount );
        
        vkGetPhysicalDeviceQueueFamilyProperties( GfxDeviceGlobal::physicalDevice, &queueCount, queueProps.elements );
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
        Array< VkExtensionProperties > availableDeviceExtensions;
        availableDeviceExtensions.Allocate( deviceExtensionCount );
        
        vkEnumerateDeviceExtensionProperties( GfxDeviceGlobal::physicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtensions.elements );

        debug::hasMarker = true;

        vkGetPhysicalDeviceFeatures( GfxDeviceGlobal::physicalDevice, &GfxDeviceGlobal::deviceFeatures );

        VkPhysicalDeviceFeatures enabledFeatures = {};
        enabledFeatures.tessellationShader = GfxDeviceGlobal::deviceFeatures.tessellationShader;
        enabledFeatures.shaderTessellationAndGeometryPointSize = GfxDeviceGlobal::deviceFeatures.shaderTessellationAndGeometryPointSize;
        enabledFeatures.shaderClipDistance = GfxDeviceGlobal::deviceFeatures.shaderClipDistance;
        enabledFeatures.shaderCullDistance = GfxDeviceGlobal::deviceFeatures.shaderCullDistance;
        enabledFeatures.textureCompressionBC = GfxDeviceGlobal::deviceFeatures.textureCompressionBC;
        enabledFeatures.fillModeNonSolid = GfxDeviceGlobal::deviceFeatures.fillModeNonSolid;
        enabledFeatures.samplerAnisotropy = GfxDeviceGlobal::deviceFeatures.samplerAnisotropy;
        enabledFeatures.fragmentStoresAndAtomics = GfxDeviceGlobal::deviceFeatures.fragmentStoresAndAtomics;
        enabledFeatures.vertexPipelineStoresAndAtomics = GfxDeviceGlobal::deviceFeatures.vertexPipelineStoresAndAtomics;
        enabledFeatures.shaderStorageImageMultisample = GfxDeviceGlobal::deviceFeatures.shaderStorageImageMultisample;
        
        if (debug::enabled)
        {
            enabledFeatures.robustBufferAccess = GfxDeviceGlobal::deviceFeatures.robustBufferAccess;
        }
        
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
        deviceCreateInfo.enabledExtensionCount = static_cast< std::uint32_t >( deviceExtensions.size() );
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        result = vkCreateDevice( GfxDeviceGlobal::physicalDevice, &deviceCreateInfo, nullptr, &GfxDeviceGlobal::device );
        AE3D_CHECK_VULKAN( result, "device" );

        vkGetPhysicalDeviceMemoryProperties( GfxDeviceGlobal::physicalDevice, &GfxDeviceGlobal::deviceMemoryProperties );
        vkGetDeviceQueue( GfxDeviceGlobal::device, graphicsQueueIndex, 0, &GfxDeviceGlobal::graphicsQueue );

        const VkFormat depthFormats[ 4 ] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
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

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = samplerInfo.magFilter;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = samplerInfo.addressModeU;
        samplerInfo.addressModeW = samplerInfo.addressModeU;
        samplerInfo.mipLodBias = 0;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerInfo.minLod = 0;
        samplerInfo.maxLod = 1.0f;
        samplerInfo.maxAnisotropy = 1;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        VkResult err = vkCreateSampler( GfxDeviceGlobal::device, &samplerInfo, nullptr, &GfxDeviceGlobal::linearRepeat );
        AE3D_CHECK_VULKAN( err, "vkCreateSampler" );
        Texture2DGlobal::samplersToReleaseAtExit.push_back( GfxDeviceGlobal::linearRepeat );

        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::linearRepeat, VK_OBJECT_TYPE_SAMPLER, "linearRepeat" );
    }

    void CreateFramebufferNonMSAA()
    {
        VkImageView attachments[ 2 ];

        // Depth/Stencil attachment is the same for all frame buffers
        attachments[ 1 ] = GfxDeviceGlobal::depthStencil.view;

        VkFramebufferCreateInfo frameBufferCreateInfo = {};
        frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCreateInfo.renderPass = GfxDeviceGlobal::renderPass;
        frameBufferCreateInfo.attachmentCount = 2;
        frameBufferCreateInfo.pAttachments = attachments;
        frameBufferCreateInfo.width = static_cast< std::uint32_t >( WindowGlobal::windowWidth );
        frameBufferCreateInfo.height = static_cast< std::uint32_t >( WindowGlobal::windowHeight );
        frameBufferCreateInfo.layers = 1;

        // Create frame buffers for every swap chain image
        GfxDeviceGlobal::frameBuffers.Allocate( GfxDeviceGlobal::swapchainBuffers.count );
        
        for (unsigned i = 0; i < GfxDeviceGlobal::frameBuffers.count; ++i)
        {
            attachments[ 0 ] = GfxDeviceGlobal::swapchainBuffers[ i ].view;
            VkResult err = vkCreateFramebuffer( GfxDeviceGlobal::device, &frameBufferCreateInfo, nullptr, &GfxDeviceGlobal::frameBuffers[ i ] );
            debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::frameBuffers[ i ], VK_OBJECT_TYPE_FRAMEBUFFER, "framebuffer" );

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
        frameBufferCreateInfo.renderPass = GfxDeviceGlobal::renderPass;
        frameBufferCreateInfo.attachmentCount = 4;
        frameBufferCreateInfo.pAttachments = attachments;
        frameBufferCreateInfo.width = static_cast< std::uint32_t >(WindowGlobal::windowWidth);
        frameBufferCreateInfo.height = static_cast< std::uint32_t >(WindowGlobal::windowHeight);
        frameBufferCreateInfo.layers = 1;

        // Create frame buffers for every swap chain image
        GfxDeviceGlobal::frameBuffers.Allocate( GfxDeviceGlobal::swapchainBuffers.count );

        for (unsigned i = 0; i < GfxDeviceGlobal::frameBuffers.count; ++i)
        {
            attachments[ 1 ] = GfxDeviceGlobal::swapchainBuffers[ i ].view;
            VkResult err = vkCreateFramebuffer( GfxDeviceGlobal::device, &frameBufferCreateInfo, nullptr, &GfxDeviceGlobal::frameBuffers[ i ] );
            AE3D_CHECK_VULKAN( err, "vkCreateFramebuffer" );
        }
    }

    VkSampleCountFlagBits GetSampleBits( int msaaSampleCount )
    {
        return (VkSampleCountFlagBits)msaaSampleCount;
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
        attachments[ 1 ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorReference;
        subpass.pDepthStencilAttachment = &depthReference;
        subpass.preserveAttachmentCount = 0;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 0;
        renderPassInfo.pDependencies = nullptr;

        VkResult err = vkCreateRenderPass( GfxDeviceGlobal::device, &renderPassInfo, nullptr, &GfxDeviceGlobal::renderPass );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::renderPass, VK_OBJECT_TYPE_RENDER_PASS, "renderpass nonMSAA" );

        AE3D_CHECK_VULKAN( err, "vkCreateRenderPass" );   
    }

    void CreateRenderPassMSAA()
    {
        VkAttachmentDescription attachments[ 4 ];

        // Multisampled attachment that we render to
        attachments[ 0 ].format = GfxDeviceGlobal::colorFormat;
        attachments[ 0 ].samples = GfxDeviceGlobal::msaaSampleBits;
        attachments[ 0 ].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[ 0 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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
        attachments[ 2 ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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

        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::renderPass, VK_OBJECT_TYPE_RENDER_PASS, "renderPass MSAA" );
    }

    void CreateDepthStencil()
    {
        VkImageCreateInfo image = {};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = GfxDeviceGlobal::depthFormat;
        image.extent = { static_cast< std::uint32_t >( WindowGlobal::windowWidth ), static_cast< std::uint32_t >( WindowGlobal::windowHeight ), 1 };
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image.flags = 0;

        VkMemoryAllocateInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;

        VkImageViewCreateInfo depthStencilView = {};
        depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
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
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::depthStencil.image, VK_OBJECT_TYPE_IMAGE, "depthstencil" );

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.image, &memReqs );
        mem_alloc.allocationSize = memReqs.size;
        mem_alloc.memoryTypeIndex = GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        err = vkAllocateMemory( GfxDeviceGlobal::device, &mem_alloc, nullptr, &GfxDeviceGlobal::depthStencil.mem );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::depthStencil.mem, VK_OBJECT_TYPE_DEVICE_MEMORY, "GfxDeviceGlobal::depthStencil.mem" );

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
        Statistics::IncQueueSubmitCalls();

        err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
        AE3D_CHECK_VULKAN( err, "vkQueueWaitIdle" );

        vkFreeCommandBuffers( GfxDeviceGlobal::device, GfxDeviceGlobal::cmdPool, 1, &GfxDeviceGlobal::setupCmdBuffer );
        GfxDeviceGlobal::setupCmdBuffer = VK_NULL_HANDLE;
    }

    void CreateDescriptorPool()
    {
        const int AE3D_DESCRIPTOR_SETS_COUNT = 5550;

        const VkDescriptorPoolSize typeCounts[ descriptorSlotCount ] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_SAMPLER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_SAMPLER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, AE3D_DESCRIPTOR_SETS_COUNT },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, AE3D_DESCRIPTOR_SETS_COUNT }
        };

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = descriptorSlotCount;
        descriptorPoolInfo.pPoolSizes = typeCounts;
        descriptorPoolInfo.maxSets = AE3D_DESCRIPTOR_SETS_COUNT;
        descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        VkResult err = vkCreateDescriptorPool( GfxDeviceGlobal::device, &descriptorPoolInfo, nullptr, &GfxDeviceGlobal::descriptorPool );
        AE3D_CHECK_VULKAN( err, "vkCreateDescriptorPool" );

        GfxDeviceGlobal::descriptorSets.Allocate( AE3D_DESCRIPTOR_SETS_COUNT );

        for (unsigned i = 0; i < GfxDeviceGlobal::descriptorSets.count; ++i)
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

    VkDescriptorSet AllocateDescriptorSet( const VkDescriptorBufferInfo& uboDesc, const VkImageView& view0, VkSampler sampler0, const VkImageView& view1, VkSampler sampler1, const VkImageView& view2, const VkImageView& view3, const VkImageView& view4, const VkImageView& view14 )
    {
        VkDescriptorSet outDescriptorSet = GfxDeviceGlobal::descriptorSets[ GfxDeviceGlobal::descriptorSetIndex ];
        GfxDeviceGlobal::descriptorSetIndex = (GfxDeviceGlobal::descriptorSetIndex + 1) % GfxDeviceGlobal::descriptorSets.count;

        VkDescriptorImageInfo sampler0Desc = {};
        sampler0Desc.sampler = sampler0;
        sampler0Desc.imageView = view0;
        sampler0Desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo sampler1Desc = {};
        sampler1Desc.sampler = sampler1;
        sampler1Desc.imageView = view1;
        sampler1Desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet sets[ descriptorSlotCount ] = {};

        // Binding 0 : Image
        sets[ 0 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 0 ].dstSet = outDescriptorSet;
        sets[ 0 ].descriptorCount = 1;
        sets[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sets[ 0 ].pImageInfo = &sampler0Desc;
        sets[ 0 ].dstBinding = 0;

        // Binding 1 : Image
        sets[ 1 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 1 ].dstSet = outDescriptorSet;
        sets[ 1 ].descriptorCount = 1;
        sets[ 1 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sets[ 1 ].pImageInfo = &sampler1Desc;
        sets[ 1 ].dstBinding = 1;

        VkDescriptorImageInfo sampler2Desc = {};
        sampler2Desc.sampler = sampler1;
        sampler2Desc.imageView = view2;
        sampler2Desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Binding 2 : Image
        sets[ 2 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 2 ].dstSet = outDescriptorSet;
        sets[ 2 ].descriptorCount = 1;
        sets[ 2 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sets[ 2 ].pImageInfo = &sampler2Desc;
        sets[ 2 ].dstBinding = 2;

        VkDescriptorImageInfo sampler3Desc = {};
        sampler3Desc.sampler = sampler1;
        sampler3Desc.imageView = view3;
        sampler3Desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Binding 3 : Image
        sets[ 3 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 3 ].dstSet = outDescriptorSet;
        sets[ 3 ].descriptorCount = 1;
        sets[ 3 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sets[ 3 ].pImageInfo = &sampler3Desc;
        sets[ 3 ].dstBinding = 3;

        VkDescriptorImageInfo sampler4Desc = {};
        sampler4Desc.sampler = sampler0;
        sampler4Desc.imageView = view4;
        sampler4Desc.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Binding 4 : Image
        sets[ 4 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 4 ].dstSet = outDescriptorSet;
        sets[ 4 ].descriptorCount = 1;
        sets[ 4 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        sets[ 4 ].pImageInfo = &sampler4Desc;
        sets[ 4 ].dstBinding = 4;

        // Binding 5 : Sampler
        sets[ 5 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 5 ].dstSet = outDescriptorSet;
        sets[ 5 ].descriptorCount = 1;
        sets[ 5 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        sets[ 5 ].pImageInfo = &sampler0Desc;
        sets[ 5 ].dstBinding = 5;

        // Binding 6 : Sampler
        sets[ 6 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 6 ].dstSet = outDescriptorSet;
        sets[ 6 ].descriptorCount = 1;
        sets[ 6 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        sets[ 6 ].pImageInfo = &sampler1Desc;
        sets[ 6 ].dstBinding = 6;

        // Binding 7 : Buffer
        sets[ 7 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 7 ].dstSet = outDescriptorSet;
        sets[ 7 ].descriptorCount = 1;
        sets[ 7 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sets[ 7 ].pBufferInfo = &uboDesc;
        sets[ 7 ].dstBinding = 7;

        // Binding 8 : Buffer
        sets[ 8 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 8 ].dstSet = outDescriptorSet;
        sets[ 8 ].descriptorCount = 1;
        sets[ 8 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        sets[ 8 ].pTexelBufferView = GfxDeviceGlobal::lightTiler.GetPointLightBufferView();
        sets[ 8 ].dstBinding = 8;

        // Binding 9 : Buffer (UAV)
        sets[ 9 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 9 ].dstSet = outDescriptorSet;
        sets[ 9 ].descriptorCount = 1;
        sets[ 9 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        sets[ 9 ].pTexelBufferView = GfxDeviceGlobal::lightTiler.GetLightIndexBufferView();
        sets[ 9 ].dstBinding = 9;

        // Binding 10 : Buffer
        sets[ 10 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 10 ].dstSet = outDescriptorSet;
        sets[ 10 ].descriptorCount = 1;
        sets[ 10 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        sets[ 10 ].pTexelBufferView = GfxDeviceGlobal::lightTiler.GetPointLightColorBufferView();
        sets[ 10 ].dstBinding = 10;

        // Binding 11 : Buffer
        sets[ 11 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 11 ].dstSet = outDescriptorSet;
        sets[ 11 ].descriptorCount = 1;
        sets[ 11 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        sets[ 11 ].pTexelBufferView = GfxDeviceGlobal::lightTiler.GetSpotLightBufferView();
        sets[ 11 ].dstBinding = 11;

		// Binding 12 : Buffer
        sets[ 12 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 12 ].dstSet = outDescriptorSet;
        sets[ 12 ].descriptorCount = 1;
        sets[ 12 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        sets[ 12 ].pTexelBufferView = GfxDeviceGlobal::lightTiler.GetSpotLightParamsView();
        sets[ 12 ].dstBinding = 12;

        // Binding 13 : Buffer
        sets[ 13 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 13 ].dstSet = outDescriptorSet;
        sets[ 13 ].descriptorCount = 1;
        sets[ 13 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        sets[ 13 ].pTexelBufferView = GfxDeviceGlobal::lightTiler.GetSpotLightColorBufferView();
        sets[ 13 ].dstBinding = 13;

        VkDescriptorImageInfo sampler14Desc = {};
        sampler14Desc.sampler = sampler1;
        sampler14Desc.imageView = view14;
        sampler14Desc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        // Binding 14 : Writable texture
        sets[ 14 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 14 ].dstSet = outDescriptorSet;
        sets[ 14 ].descriptorCount = 1;
        sets[ 14 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        sets[ 14 ].pImageInfo = &sampler14Desc;
        sets[ 14 ].dstBinding = 14;

        VkDescriptorBufferInfo bufferDesc = {};
        bufferDesc.buffer = particleBuffer;
        bufferDesc.range = VK_WHOLE_SIZE;

        // Binding 15 : Particle buffer.
        sets[ 15 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 15 ].dstSet = outDescriptorSet;
        sets[ 15 ].descriptorCount = 1;
        sets[ 15 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        sets[ 15 ].pBufferInfo = &bufferDesc;
        sets[ 15 ].dstBinding = 15;

        // Binding 16 : Particle tile buffer.
        sets[ 16 ].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sets[ 16 ].dstSet = outDescriptorSet;
        sets[ 16 ].descriptorCount = 1;
        sets[ 16 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        sets[ 16 ].pTexelBufferView = &particleTileBufferView;
        sets[ 16 ].dstBinding = 16;

        vkUpdateDescriptorSets( GfxDeviceGlobal::device, descriptorSlotCount, sets, 0, nullptr );

        return outDescriptorSet;
    }

    void CreateDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding layoutBindings[ descriptorSlotCount ] = {};

        // Binding 0 : Image
        layoutBindings[ 0 ].binding = 0;
        layoutBindings[ 0 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        layoutBindings[ 0 ].descriptorCount = 1;
        layoutBindings[ 0 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 1 : Image
        layoutBindings[ 1 ].binding = 1;
        layoutBindings[ 1 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        layoutBindings[ 1 ].descriptorCount = 1;
        layoutBindings[ 1 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 2 : Image
        layoutBindings[ 2 ].binding = 2;
        layoutBindings[ 2 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        layoutBindings[ 2 ].descriptorCount = 1;
        layoutBindings[ 2 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 3 : Image
        layoutBindings[ 3 ].binding = 3;
        layoutBindings[ 3 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        layoutBindings[ 3 ].descriptorCount = 1;
        layoutBindings[ 3 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 4 : Image
        layoutBindings[ 4 ].binding = 4;
        layoutBindings[ 4 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        layoutBindings[ 4 ].descriptorCount = 1;
        layoutBindings[ 4 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 5 : Sampler
        layoutBindings[ 5 ].binding = 5;
        layoutBindings[ 5 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        layoutBindings[ 5 ].descriptorCount = 1;
        layoutBindings[ 5 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 6 : Sampler
        layoutBindings[ 6 ].binding = 6;
        layoutBindings[ 6 ].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        layoutBindings[ 6 ].descriptorCount = 1;
        layoutBindings[ 6 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 7 : Uniform buffer
        layoutBindings[ 7 ].binding = 7;
        layoutBindings[ 7 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBindings[ 7 ].descriptorCount = 1;
        layoutBindings[ 7 ].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 8 : Buffer
        layoutBindings[ 8 ].binding = 8;
        layoutBindings[ 8 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        layoutBindings[ 8 ].descriptorCount = 1;
        layoutBindings[ 8 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 9 : RWBuffer
        layoutBindings[ 9 ].binding = 9;
        layoutBindings[ 9 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        layoutBindings[ 9 ].descriptorCount = 1;
        layoutBindings[ 9 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 10 : Buffer
        layoutBindings[ 10 ].binding = 10;
        layoutBindings[ 10 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        layoutBindings[ 10 ].descriptorCount = 1;
        layoutBindings[ 10 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding 11 : Buffer
        layoutBindings[ 11 ].binding = 11;
        layoutBindings[ 11 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        layoutBindings[ 11 ].descriptorCount = 1;
        layoutBindings[ 11 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 12 : Buffer
        layoutBindings[ 12 ].binding = 12;
        layoutBindings[ 12 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        layoutBindings[ 12 ].descriptorCount = 1;
        layoutBindings[ 12 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 13 : Buffer
        layoutBindings[ 13 ].binding = 13;
        layoutBindings[ 13 ].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
        layoutBindings[ 13 ].descriptorCount = 1;
        layoutBindings[ 13 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // Binding 14 : Writable texture
        layoutBindings[ 14 ].binding = 14;
        layoutBindings[ 14 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        layoutBindings[ 14 ].descriptorCount = 1;
        layoutBindings[ 14 ].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 15 : Particle
        layoutBindings[ 15 ].binding = 15;
        layoutBindings[ 15 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layoutBindings[ 15 ].descriptorCount = 1;
        layoutBindings[ 15 ].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        // Binding 16 : Particle tile buffer
        layoutBindings[ 16 ].binding = 16;
        layoutBindings[ 16 ].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        layoutBindings[ 16 ].descriptorCount = 1;
        layoutBindings[ 16 ].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo descriptorLayout = {};
        descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorLayout.bindingCount = descriptorSlotCount;
        descriptorLayout.pBindings = layoutBindings;

        VkResult err = vkCreateDescriptorSetLayout( GfxDeviceGlobal::device, &descriptorLayout, nullptr, &GfxDeviceGlobal::descriptorSetLayout );
        AE3D_CHECK_VULKAN( err, "vkCreateDescriptorSetLayout" );

        VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
        pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pPipelineLayoutCreateInfo.setLayoutCount = 1;
        pPipelineLayoutCreateInfo.pSetLayouts = &GfxDeviceGlobal::descriptorSetLayout;

        err = vkCreatePipelineLayout( GfxDeviceGlobal::device, &pPipelineLayoutCreateInfo, nullptr, &GfxDeviceGlobal::pipelineLayout );
        AE3D_CHECK_VULKAN( err, "vkCreatePipelineLayout" );
    }

    void CreateSemaphores()
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult err = vkCreateSemaphore( GfxDeviceGlobal::device, &semaphoreCreateInfo, nullptr, &GfxDeviceGlobal::presentCompleteSemaphore );
        AE3D_CHECK_VULKAN( err, "vkCreateSemaphore" );

        err = vkCreateSemaphore( GfxDeviceGlobal::device, &semaphoreCreateInfo, nullptr, &GfxDeviceGlobal::renderCompleteSemaphore );
        AE3D_CHECK_VULKAN( err, "vkCreateSemaphore" );
    }
    
    void CreateRenderer( int samples, bool apiValidation )
    {
        renderer.GenerateSSAOKernel( 16, GfxDeviceGlobal::perObjectUboStruct.kernelOffsets );
        GfxDeviceGlobal::perObjectUboStruct.kernelSize = 16;
        
        GfxDeviceGlobal::msaaSampleBits = GetSampleBits( samples );
        CreateInstance( &GfxDeviceGlobal::instance );

        if (apiValidation)
        {
            debug::enabled = true;
        }
        
        debug::Setup( GfxDeviceGlobal::instance );

        CreateDevice();

        debug::SetupDevice( GfxDeviceGlobal::device );

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

        VkQueryPoolCreateInfo queryPoolInfo = {};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = 2;

        err = vkCreateQueryPool( GfxDeviceGlobal::device, &queryPoolInfo, nullptr, &GfxDeviceGlobal::queryPool );
        AE3D_CHECK_VULKAN( err, "vkCreateQueryPool" );

        GfxDeviceGlobal::uiVertexBuffer.GenerateDynamic( UI_FACE_COUNT, UI_VERTICE_COUNT );
        
        GfxDeviceGlobal::lightTiler.Init();

        VkCommandBufferAllocateInfo cmdBufInfo = {};
        cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufInfo.commandPool = GfxDeviceGlobal::cmdPool;
        cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufInfo.commandBufferCount = 1;

        err = vkAllocateCommandBuffers( GfxDeviceGlobal::device, &cmdBufInfo, &GfxDeviceGlobal::texCmdBuffer );
        AE3D_CHECK_VULKAN( err, "vkAllocateCommandBuffers" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)GfxDeviceGlobal::texCmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "texCmdBuffer" );
    }
}

void BindComputeDescriptorSet()
{
    VkDescriptorSet descriptorSet = ae3d::AllocateDescriptorSet( GfxDeviceGlobal::ubos[ GfxDeviceGlobal::currentUbo ].uboDesc, GfxDeviceGlobal::boundViews[ 0 ], GfxDeviceGlobal::boundSamplers[ 0 ],
                                                                 GfxDeviceGlobal::boundViews[ 1 ], GfxDeviceGlobal::boundSamplers[ 1 ], GfxDeviceGlobal::boundViews[ 2 ], GfxDeviceGlobal::boundViews[ 3 ], GfxDeviceGlobal::boundViews[ 4 ], GfxDeviceGlobal::boundViews[ 14 ] );

    vkCmdBindDescriptorSets( GfxDeviceGlobal::computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                             GfxDeviceGlobal::pipelineLayout, 0, 1, &descriptorSet, 0, nullptr );
}

void UploadPerObjectUbo()
{
    std::memcpy( &ae3d::GfxDevice::GetCurrentUbo()[ 0 ], &GfxDeviceGlobal::perObjectUboStruct, sizeof( GfxDeviceGlobal::perObjectUboStruct ) );
}

void ae3d::GfxDevice::Init( int width, int height )
{
    GfxDevice::backBufferWidth = width;
    GfxDevice::backBufferHeight = height;
}

void ae3d::GfxDevice::DrawUI( int scX, int scY, int scWidth, int scHeight, int elemCount, int offset )
{
    int scissor[ 4 ] = {};
    scissor[ 0 ] = scX < 0 ? 0 : scX;
    scissor[ 1 ] = scY < 0 ? 0 : scY;
    scissor[ 2 ] = scWidth > 8191 ? 8191 : scWidth;
    scissor[ 3 ] = scHeight > 8191 ? 8191 : scHeight;
    SetScissor( scissor );
    
    Draw( GfxDeviceGlobal::uiVertexBuffer, offset, offset + elemCount, renderer.builtinShaders.uiShader, BlendMode::AlphaBlend, DepthFunc::NoneWriteOff, CullMode::Off, FillMode::Solid, GfxDevice::PrimitiveTopology::Triangles );
}

void ae3d::GfxDevice::ResetPSOCache()
{
    GfxDeviceGlobal::psoCache.clear();
}

void ae3d::GfxDevice::MapUIVertexBuffer( int /*vertexSize*/, int /*indexSize*/, void** outMappedVertices, void** outMappedIndices )
{
    *outMappedVertices = GfxDeviceGlobal::uiVertices;
    *outMappedIndices = GfxDeviceGlobal::uiFaces;
}

void ae3d::GfxDevice::UnmapUIVertexBuffer()
{
    GfxDeviceGlobal::uiVertexBuffer.UpdateDynamic( GfxDeviceGlobal::uiFaces, UI_FACE_COUNT, GfxDeviceGlobal::uiVertices, UI_VERTICE_COUNT );
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

void ae3d::GfxDevice::BeginLightCullerGpuQuery()
{
}

void ae3d::GfxDevice::EndLightCullerGpuQuery()
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
    VkClearValue clearValues[ 3 ];
    if (GfxDeviceGlobal::msaaSampleBits != VK_SAMPLE_COUNT_1_BIT)
    {
        clearValues[ 0 ].color = GfxDeviceGlobal::clearColor;
        clearValues[ 1 ].color = GfxDeviceGlobal::clearColor;
        clearValues[ 2 ].depthStencil = { 1.0f, 0 };
    }
    else
    {
        clearValues[ 0 ].color = GfxDeviceGlobal::clearColor;
        clearValues[ 1 ].depthStencil = { 1.0f, 0 };
    }

    const uint32_t width = GfxDeviceGlobal::renderTexture0 ? GfxDeviceGlobal::renderTexture0->GetWidth() : WindowGlobal::windowWidth;
    const uint32_t height = GfxDeviceGlobal::renderTexture0 ? GfxDeviceGlobal::renderTexture0->GetHeight() : WindowGlobal::windowHeight;

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::currentCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = GfxDeviceGlobal::renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = GfxDeviceGlobal::msaaSampleBits != VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    renderPassBeginInfo.pClearValues = clearValues;
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

void ae3d::GfxDevice::BeginRenderPass()
{
    const uint32_t width = GfxDeviceGlobal::renderTexture0 ? GfxDeviceGlobal::renderTexture0->GetWidth() : WindowGlobal::windowWidth;
    const uint32_t height = GfxDeviceGlobal::renderTexture0 ? GfxDeviceGlobal::renderTexture0->GetHeight() : WindowGlobal::windowHeight;

    VkClearValue clearValues[ 3 ];
    
    if (GfxDeviceGlobal::msaaSampleBits != VK_SAMPLE_COUNT_1_BIT)
    {
        clearValues[ 0 ].color = GfxDeviceGlobal::clearColor;
        clearValues[ 1 ].color = GfxDeviceGlobal::clearColor;
        clearValues[ 2 ].depthStencil = { 1.0f, 0 };
    }
    else
    {
        clearValues[ 0 ].color = GfxDeviceGlobal::clearColor;
        clearValues[ 1 ].depthStencil = { 1.0f, 0 };
    }

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = GfxDeviceGlobal::renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = width;
    renderPassBeginInfo.renderArea.extent.height = height;
    renderPassBeginInfo.clearValueCount = GfxDeviceGlobal::msaaSampleBits != VK_SAMPLE_COUNT_1_BIT ? 3 : 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = GfxDeviceGlobal::frameBuffers[ GfxDeviceGlobal::currentBuffer ];

    vkCmdBeginRenderPass( GfxDeviceGlobal::currentCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
}

void ae3d::GfxDevice::EndRenderPass()
{
    vkCmdEndRenderPass( GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ] );
}

void ae3d::GfxDevice::EndCommandBuffer()
{
    VkResult err = vkEndCommandBuffer( GfxDeviceGlobal::currentCmdBuffer );
    AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );
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

    Draw( GfxDeviceGlobal::lineBuffers[ handle ], 0, GfxDeviceGlobal::lineBuffers[ handle ].GetFaceCount() / 3, shader, BlendMode::Off, DepthFunc::LessOrEqualWriteOn, CullMode::Off, FillMode::Solid, GfxDevice::PrimitiveTopology::Lines );
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

static void PrintShaderStatistics( VkPipeline pso )
{
    VkShaderStatisticsInfoAMD statistics = {};
    size_t dataSize = sizeof( statistics );

    if (getShaderInfoAMD && getShaderInfoAMD( GfxDeviceGlobal::device, pso,
                                              VK_SHADER_STAGE_FRAGMENT_BIT, VK_SHADER_INFO_TYPE_STATISTICS_AMD,
                                              &dataSize, &statistics ) == VK_SUCCESS)
    {
        ae3d::System::Print( "VGPR usage: %d\n", statistics.resourceUsage.numUsedVgprs );
        ae3d::System::Print( "SGPR usage: %d\n", statistics.resourceUsage.numUsedSgprs );
    }
}

void ae3d::GfxDevice::Draw( VertexBuffer& vertexBuffer, int startIndex, int endIndex, Shader& shader, BlendMode blendMode, DepthFunc depthFunc,
                            CullMode cullMode, FillMode fillMode, PrimitiveTopology topology )
{
    System::Assert( startIndex > -1 && startIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in startIndex" );
    System::Assert( endIndex > -1 && endIndex >= startIndex && endIndex <= vertexBuffer.GetFaceCount() / 3, "Invalid vertex buffer draw range in endIndex" );
    System::Assert( GfxDeviceGlobal::currentBuffer < GfxDeviceGlobal::swapchainBuffers.count, "invalid draw buffer index" );

    if (GfxDeviceGlobal::boundViews[ 0 ] == VK_NULL_HANDLE || GfxDeviceGlobal::boundSamplers[ 0 ] == VK_NULL_HANDLE)
    {
        return;
    }

    if (shader.GetVertexInfo().module == VK_NULL_HANDLE || shader.GetFragmentInfo().module == VK_NULL_HANDLE)
    {
        return;
    }

    if (Statistics::GetDrawCalls() >= (int)GfxDeviceGlobal::descriptorSets.count)
    {
        System::Print( "Skipping draw because draw call count %d exceeds descriptor set count %ld \n", Statistics::GetDrawCalls(), GfxDeviceGlobal::descriptorSets.count );
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

    GfxDeviceGlobal::perObjectUboStruct.windowWidth = GfxDevice::backBufferWidth;
    GfxDeviceGlobal::perObjectUboStruct.windowHeight = GfxDevice::backBufferHeight;
    GfxDeviceGlobal::perObjectUboStruct.numLights = lightCount;
    GfxDeviceGlobal::perObjectUboStruct.maxNumLightsPerTile = GfxDeviceGlobal::lightTiler.GetMaxNumLightsPerTile();
    GfxDeviceGlobal::perObjectUboStruct.tilesXY.x = (float)GfxDeviceGlobal::lightTiler.GetNumTilesX();
    GfxDeviceGlobal::perObjectUboStruct.tilesXY.y = (float)GfxDeviceGlobal::lightTiler.GetNumTilesY();

    UploadPerObjectUbo();

    VkDescriptorSet descriptorSet = AllocateDescriptorSet( GfxDeviceGlobal::ubos[ GfxDeviceGlobal::currentUbo ].uboDesc, GfxDeviceGlobal::boundViews[ 0 ], GfxDeviceGlobal::boundSamplers[ 0 ], GfxDeviceGlobal::boundViews[ 1 ],
                                                           GfxDeviceGlobal::boundSamplers[ 1 ], GfxDeviceGlobal::boundViews[ 2 ], GfxDeviceGlobal::boundViews[ 3 ], GfxDeviceGlobal::boundViews[ 4 ], GfxDeviceGlobal::boundViews[ 14 ] );

    vkCmdBindDescriptorSets( GfxDeviceGlobal::currentCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                             GfxDeviceGlobal::pipelineLayout, 0, 1, &descriptorSet, 0, nullptr );

    VkPipeline pso = GfxDeviceGlobal::psoCache[ psoHash ];
    
    if (GfxDeviceGlobal::cachedPSO != pso)
    {
        GfxDeviceGlobal::cachedPSO = pso;
        vkCmdBindPipeline( GfxDeviceGlobal::currentCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pso );
        Statistics::IncPSOBindCalls();
    }

    VkDeviceSize offsets[ 1 ] = { 0 };
    vkCmdBindVertexBuffers( GfxDeviceGlobal::currentCmdBuffer, VertexBuffer::VERTEX_BUFFER_BIND_ID, 1, vertexBuffer.GetVertexBuffer(), offsets );

    if (topology == PrimitiveTopology::Triangles)
    {
        vkCmdBindIndexBuffer( GfxDeviceGlobal::currentCmdBuffer, *vertexBuffer.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT16 );
        vkCmdDrawIndexed( GfxDeviceGlobal::currentCmdBuffer, (endIndex - startIndex) * 3, 1, startIndex * 3, 0, 0 );
    }
    else if (topology == PrimitiveTopology::Lines)
    {
        vkCmdDraw( GfxDeviceGlobal::currentCmdBuffer, (endIndex - startIndex) * 3, 1, startIndex * 3, 0 );
    }

    Statistics::IncTriangleCount( endIndex - startIndex );
    Statistics::IncDrawCalls();

    GfxDeviceGlobal::boundViews[ 4 ] = TextureCube::GetDefaultTexture()->GetView();
}

void ae3d::GfxDevice::GetNewUniformBuffer()
{
    GfxDeviceGlobal::currentUbo = (GfxDeviceGlobal::currentUbo + 1) % GfxDeviceGlobal::ubos.count;
}

void ae3d::GfxDevice::CreateUniformBuffers()
{
    GfxDeviceGlobal::ubos.Allocate( 1800 );

    for (unsigned uboIndex = 0; uboIndex < GfxDeviceGlobal::ubos.count; ++uboIndex)
    {
        auto& ubo = GfxDeviceGlobal::ubos[ uboIndex ];

        const VkDeviceSize uboSize = 256 * 3 + 80 * 64 + 128 * 16;
        static_assert( uboSize >= sizeof( PerObjectUboStruct ), "UBO size must be larger than UBO struct" );

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = uboSize;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VkResult err = vkCreateBuffer( GfxDeviceGlobal::device, &bufferInfo, nullptr, &ubo.ubo );
        AE3D_CHECK_VULKAN( err, "vkCreateBuffer UBO" );
        debug::SetObjectName( GfxDeviceGlobal::device, (std::uint64_t)ubo.ubo, VK_OBJECT_TYPE_BUFFER, "ubo" );

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements( GfxDeviceGlobal::device, ubo.ubo, &memReqs );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
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
    GfxDeviceGlobal::cachedPSO = VK_NULL_HANDLE;
    
    SubmitPostPresentBarrier();

    GfxDeviceGlobal::boundViews[ 0 ] = Texture2D::GetDefaultTexture()->GetView();
    GfxDeviceGlobal::boundViews[ 1 ] = Texture2D::GetDefaultTexture()->GetView();
    GfxDeviceGlobal::boundViews[ 2 ] = Texture2D::GetDefaultTexture()->GetView();
    GfxDeviceGlobal::boundViews[ 3 ] = Texture2D::GetDefaultTexture()->GetView();
    GfxDeviceGlobal::boundViews[ 4 ] = TextureCube::GetDefaultTexture()->GetView();
    GfxDeviceGlobal::boundViews[ 11 ] = Texture2D::GetDefaultTexture()->GetView();
    GfxDeviceGlobal::boundViews[ 14 ] = Texture2D::GetDefaultTextureUAV()->GetView();
    GfxDeviceGlobal::boundSamplers[ 0 ] = Texture2D::GetDefaultTexture()->GetSampler();
    GfxDeviceGlobal::boundSamplers[ 1 ] = GfxDeviceGlobal::linearRepeat;
}

void SubmitQueue()
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
    Statistics::IncQueueSubmitCalls();
}

void ae3d::GfxDevice::Present()
{
    Statistics::BeginPresentTimeProfiling();
    VkResult err = VK_SUCCESS;

#if AE3D_OPENVR
    VR::SubmitFrame();
#else
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

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );
    Statistics::IncQueueSubmitCalls();
#endif

    SubmitPrePresentBarrier();

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
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

    // FIXME: This slows down rendering
    
    System::BeginTimer();
    err = vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
    Statistics::IncQueueWaitTime( System::EndTimer() );
    AE3D_CHECK_VULKAN( err, "vkQueueWaitIdle" );

    for (unsigned i = 0; i < GfxDeviceGlobal::pendingFreeVBs.count; ++i)
    {
        vkDestroyBuffer( GfxDeviceGlobal::device, GfxDeviceGlobal::pendingFreeVBs[ i ], nullptr );
    }

    GfxDeviceGlobal::pendingFreeVBs.Allocate( 0 );

    for (unsigned i = 0; i < GfxDeviceGlobal::pendingFreeMemory.count; ++i)
    {
        vkFreeMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::pendingFreeMemory[ i ], nullptr );
    }
    
    GfxDeviceGlobal::pendingFreeMemory.Allocate( 0 );
    Statistics::EndPresentTimeProfiling();
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    VkResult err = vkDeviceWaitIdle( GfxDeviceGlobal::device );
    AE3D_CHECK_VULKAN( err, "vkDeviceWaitIdle" );

    debug::Free( GfxDeviceGlobal::instance );
    
    for (unsigned i = 0; i < GfxDeviceGlobal::swapchainBuffers.count; ++i)
    {
        vkDestroyImageView( GfxDeviceGlobal::device, GfxDeviceGlobal::swapchainBuffers[ i ].view, nullptr );
    }

    for (unsigned i = 0; i < GfxDeviceGlobal::frameBuffers.count; ++i)
    {
        vkDestroyFramebuffer( GfxDeviceGlobal::device, GfxDeviceGlobal::frameBuffers[ i ], nullptr );
    }

    vkDestroyImage( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.image, nullptr );
    vkDestroyImageView( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.view, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, GfxDeviceGlobal::depthStencil.mem, nullptr );
    vkDestroyBuffer( GfxDeviceGlobal::device, particleBuffer, nullptr );
    vkFreeMemory( GfxDeviceGlobal::device, particleMemory, nullptr );

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

    for (unsigned i = 0; i < GfxDeviceGlobal::ubos.count; ++i)
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
    vkDestroyPipelineLayout( GfxDeviceGlobal::device, GfxDeviceGlobal::pipelineLayout, nullptr );
    vkDestroyPipelineCache( GfxDeviceGlobal::device, GfxDeviceGlobal::pipelineCache, nullptr );
    vkDestroySwapchainKHR( GfxDeviceGlobal::device, GfxDeviceGlobal::swapChain, nullptr );
    vkDestroySurfaceKHR( GfxDeviceGlobal::instance, GfxDeviceGlobal::surface, nullptr );
    vkDestroyCommandPool( GfxDeviceGlobal::device, GfxDeviceGlobal::cmdPool, nullptr );
    vkDestroyDevice( GfxDeviceGlobal::device, nullptr );
    vkDestroyInstance( GfxDeviceGlobal::instance, nullptr );
}

void ae3d::GfxDevice::SetRenderTarget( RenderTexture* target, unsigned cubeMapFace )
{
    GfxDeviceGlobal::currentCmdBuffer = target ? GfxDeviceGlobal::offscreenCmdBuffer : GfxDeviceGlobal::drawCmdBuffers[ GfxDeviceGlobal::currentBuffer ];
    GfxDeviceGlobal::cachedPSO = VK_NULL_HANDLE;
    GfxDeviceGlobal::renderTexture0 = target;

    if (target && target->IsCube())
    {
        GfxDeviceGlobal::frameBuffer0 = target->GetFrameBufferFace( cubeMapFace );
    }
    else
    {
        GfxDeviceGlobal::frameBuffer0 = target ? target->GetFrameBuffer() : VK_NULL_HANDLE;
    }
}

void BeginOffscreen()
{
    ae3d::System::Assert( GfxDeviceGlobal::renderTexture0 != nullptr, "Render texture must be set when beginning offscreen rendering" );
    
    // FIXME: Use fence instead of queue wait.
    ae3d::System::BeginTimer();
    vkQueueWaitIdle( GfxDeviceGlobal::graphicsQueue );
    Statistics::IncQueueWaitTime( ae3d::System::EndTimer() );

    VkCommandBufferBeginInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VkResult err = vkBeginCommandBuffer( GfxDeviceGlobal::offscreenCmdBuffer, &cmdBufInfo );
    AE3D_CHECK_VULKAN( err, "vkBeginCommandBuffer" );

#ifndef DISABLE_TIMESTAMPS
    vkCmdResetQueryPool( GfxDeviceGlobal::offscreenCmdBuffer, GfxDeviceGlobal::queryPool, 0, 2 );
    vkCmdWriteTimestamp( GfxDeviceGlobal::offscreenCmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, GfxDeviceGlobal::queryPool, 0 );
#endif
    
    VkClearValue clearValues[ 2 ];
    clearValues[ 0 ].color = GfxDeviceGlobal::clearColor;
    clearValues[ 1 ].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = GfxDeviceGlobal::renderTexture0->GetRenderPass();
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = GfxDeviceGlobal::renderTexture0->GetWidth();
    renderPassBeginInfo.renderArea.extent.height = GfxDeviceGlobal::renderTexture0->GetHeight();
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = GfxDeviceGlobal::frameBuffer0;

    vkCmdBeginRenderPass( GfxDeviceGlobal::offscreenCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
}

void EndOffscreen( int profilerIndex, ae3d::RenderTexture* target )
{
#ifndef DISABLE_TIMESTAMPS    
    vkCmdWriteTimestamp( GfxDeviceGlobal::offscreenCmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, GfxDeviceGlobal::queryPool, 1 );
#endif
    vkCmdEndRenderPass( GfxDeviceGlobal::offscreenCmdBuffer );
    
    VkResult err = vkEndCommandBuffer( GfxDeviceGlobal::offscreenCmdBuffer );
    AE3D_CHECK_VULKAN( err, "vkEndCommandBuffer" );

    VkPipelineStageFlags pipelineStages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &pipelineStages;
    submitInfo.waitSemaphoreCount = 0;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &GfxDeviceGlobal::offscreenCmdBuffer;

    err = vkQueueSubmit( GfxDeviceGlobal::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    AE3D_CHECK_VULKAN( err, "vkQueueSubmit" );
    Statistics::IncQueueSubmitCalls();

#ifndef DISABLE_TIMESTAMPS
    std::uint64_t timestamps[ 2 ] = {};
    err = vkGetQueryPoolResults( GfxDeviceGlobal::device, GfxDeviceGlobal::queryPool, 0, 2, sizeof( std::uint64_t ) * 2, timestamps, sizeof( std::uint64_t ), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );
    //AE3D_CHECK_VULKAN( err, "vkGetQueryPoolResults" );
    GfxDeviceGlobal::timings[ 0 ] = (timestamps[ 1 ] - timestamps[ 0 ]) / 1000.0f;
#endif
    
    if (profilerIndex == 0)
    {
        Statistics::SetDepthNormalsGpuTime( GfxDeviceGlobal::timings[ 0 ] );
    }
    else if (profilerIndex == 1)
    {
        Statistics::SetShadowMapGpuTime( GfxDeviceGlobal::timings[ 0 ] );
    }
    else if (profilerIndex == 2)
    {
        Statistics::SetPrimaryPassGpuTime( GfxDeviceGlobal::timings[ 0 ] );
    }

    if (target)
    {
        target->color.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
}

