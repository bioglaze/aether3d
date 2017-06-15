#include <vulkan/vulkan.h>
#include <cstring>
#include <cstdint>
#include "Macros.hpp"
#include "System.hpp"
#include "VertexBuffer.hpp"
#include "Shader.hpp"
#include "Statistics.hpp"
#include "GfxDevice.hpp"

namespace debug
{
#if DEBUG
    bool enabled = true;
#else
    bool enabled = false;
#endif
    bool hasMarker = false;

    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = nullptr;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = nullptr;
    PFN_vkDebugReportMessageEXT dbgBreakCallback = nullptr;
    PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectName = nullptr;
    PFN_vkCmdDebugMarkerBeginEXT CmdDebugMarkerBegin = nullptr;
    PFN_vkCmdDebugMarkerEndEXT CmdDebugMarkerEnd = nullptr;
    PFN_vkCmdDebugMarkerInsertEXT CmdDebugMarkerInsertEXT = nullptr;

    VkDebugReportCallbackEXT debugReportCallback = nullptr;

    VkBool32 messageCallback( VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*objType*/, std::uint64_t /*srcObject*/,
        std::size_t /*location*/, std::int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* /*pUserData*/ )
    {
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            ae3d::System::Print( "Vulkan error: [%s], code: %d: %s\n", pLayerPrefix, msgCode, pMsg );
            ae3d::System::Assert( false, "Vulkan error" );
        }
        else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            ae3d::System::Print( "Vulkan perf warning: [%s], code: %d: %s\n", pLayerPrefix, msgCode, pMsg );
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            ae3d::System::Print( "Vulkan warning: [%s], code: %d: %s\n", pLayerPrefix, msgCode, pMsg );
#if _MSC_VER && DEBUG
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
        AE3D_CHECK_VULKAN( err, "Unable to create debug report callback" );
    }

    void SetupDevice( VkDevice device )
    {
        DebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr( device, "vkDebugMarkerSetObjectNameEXT" );
        CmdDebugMarkerBegin = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr( device, "vkCmdDebugMarkerBeginEXT" );
        CmdDebugMarkerEnd = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr( device, "vkCmdDebugMarkerEndEXT" );
        CmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr( device, "vkCmdDebugMarkerInsertEXT" );   
    }

    void SetObjectName( VkDevice device, std::uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name )
    {
        if (DebugMarkerSetObjectName && hasMarker)
        {
            VkDebugMarkerObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.object = object;
            nameInfo.pObjectName = name;
            DebugMarkerSetObjectName( device, &nameInfo );
        }
    }

    void BeginRegion( VkCommandBuffer cmdbuffer, const char* pMarkerName, float r, float g, float b )
    {
        if (CmdDebugMarkerBegin && hasMarker)
        {
            float color[ 4 ] = { r, g, b, 1 };
            VkDebugMarkerMarkerInfoEXT markerInfo = {};
            markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
            memcpy( markerInfo.color, &color[ 0 ], sizeof( float ) * 4 );
            markerInfo.pMarkerName = pMarkerName;
            CmdDebugMarkerBegin( cmdbuffer, &markerInfo );
        }
    }

    void EndRegion( VkCommandBuffer cmdBuffer )
    {
        if (CmdDebugMarkerEnd && hasMarker)
        {
            CmdDebugMarkerEnd( cmdBuffer );
        }
    }

    void Free( VkInstance instance )
    {
        if (enabled)
        {
            DestroyDebugReportCallback( instance, debugReportCallback, nullptr );
        }
    }
}

namespace ae3d
{
    std::uint64_t GetPSOHash( ae3d::VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode,
        ae3d::GfxDevice::DepthFunc depthFunc, ae3d::GfxDevice::CullMode cullMode, ae3d::GfxDevice::FillMode fillMode, VkRenderPass renderPass )
    {
        std::uint64_t outResult = (ptrdiff_t)&vertexBuffer;
        outResult += (ptrdiff_t)&shader;
        outResult += (unsigned)blendMode;
        outResult += ((unsigned)depthFunc) * 2;
        outResult += ((unsigned)cullMode) * 4;
        outResult += ((unsigned)fillMode) * 8;
        outResult += ((ptrdiff_t)&renderPass) * 16;

        return outResult;
    }

    void CreateInstance( VkInstance* outInstance )
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Aether3D";
        appInfo.pEngineName = "Aether3D";
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = nullptr;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        std::vector< const char* > enabledExtensions;
        enabledExtensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
#if VK_USE_PLATFORM_WIN32_KHR
        enabledExtensions.push_back( VK_KHR_WIN32_SURFACE_EXTENSION_NAME );
#endif
#if VK_USE_PLATFORM_XCB_KHR
        enabledExtensions.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
#endif
        const char *validationLayerNames[] =
        {
            "VK_LAYER_LUNARG_standard_validation"
        };

        if (debug::enabled)
        {
            enabledExtensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );

            instanceCreateInfo.ppEnabledLayerNames = validationLayerNames;
            instanceCreateInfo.enabledLayerCount = 1;
        }

        instanceCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(enabledExtensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

        VkResult result = vkCreateInstance( &instanceCreateInfo, nullptr, outInstance );
        AE3D_CHECK_VULKAN( result, "instance" );
    }

    void SetImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout, unsigned layerCount, unsigned mipLevel, unsigned mipLevelCount )
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
        imageMemoryBarrier.subresourceRange.baseMipLevel = mipLevel;
        imageMemoryBarrier.subresourceRange.levelCount = mipLevelCount;
        imageMemoryBarrier.subresourceRange.layerCount = layerCount;

        if (oldImageLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
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

        if (oldImageLayout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            imageMemoryBarrier.srcAccessMask = 0;
        }

        if (newImageLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
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

        Statistics::IncBarrierCalls();
    }
}
