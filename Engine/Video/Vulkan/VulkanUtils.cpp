#include <vulkan/vulkan.h>
#include "Macros.hpp"
#include "System.hpp"
#include "VertexBuffer.hpp"
#include "Shader.hpp"
#include "GfxDevice.hpp"

namespace Stats
{
    extern int barrierCalls;
}

namespace debug
{
    bool enabled = false;
    int validationLayerCount = 1;

    const char *validationLayerNames[] =
    {
        "VK_LAYER_LUNARG_standard_validation"
    };

    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback = nullptr;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback = nullptr;
    PFN_vkDebugReportMessageEXT dbgBreakCallback = nullptr;
    PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectName = nullptr;
    PFN_vkCmdDebugMarkerBeginEXT CmdDebugMarkerBegin = nullptr;
    PFN_vkCmdDebugMarkerEndEXT CmdDebugMarkerEnd = nullptr;

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
    }

    void SetObjectName( VkDevice device, std::uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name )
    {
        if (DebugMarkerSetObjectName)
        {
            VkDebugMarkerObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.object = object;
            nameInfo.pObjectName = name;
            DebugMarkerSetObjectName( device, &nameInfo );
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

namespace MathUtil
{
    unsigned GetHash( const char* s, unsigned length );
}

namespace ae3d
{
    unsigned GetPSOHash( ae3d::VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode,
        ae3d::GfxDevice::DepthFunc depthFunc, ae3d::GfxDevice::CullMode cullMode )
    {
        std::string hashString;
        hashString += std::to_string( (ptrdiff_t)&vertexBuffer );
        hashString += std::to_string( (ptrdiff_t)&shader.GetVertexInfo().module );
        hashString += std::to_string( (ptrdiff_t)&shader.GetFragmentInfo().module );
        hashString += std::to_string( (unsigned)blendMode );
        hashString += std::to_string( ((unsigned)depthFunc) + 4 );
        hashString += std::to_string( ((unsigned)cullMode) + 8 );

        return MathUtil::GetHash( hashString.c_str(), static_cast< unsigned >(hashString.length()) );
    }

    void CreateSamplers( VkDevice device, VkSampler* linearRepeat, VkSampler* linearClamp, VkSampler* pointClamp, VkSampler* pointRepeat )
    {
        System::Assert( device != VK_NULL_HANDLE, "device not initialized" );

        VkSamplerCreateInfo sampler = {};
        sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler.pNext = nullptr;
        sampler.magFilter = VK_FILTER_LINEAR;
        sampler.minFilter = VK_FILTER_LINEAR;
        sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.mipLodBias = 0;
        sampler.compareOp = VK_COMPARE_OP_NEVER;
        sampler.minLod = 0;
        // Max level-of-detail should match mip level count
        sampler.maxLod = 0;
        sampler.maxAnisotropy = 1;
        sampler.anisotropyEnable = VK_FALSE;
        sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VkResult err = vkCreateSampler( device, &sampler, nullptr, linearRepeat );
        AE3D_CHECK_VULKAN( err, "vkCreateSampler" );

        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        err = vkCreateSampler( device, &sampler, nullptr, linearClamp );
        AE3D_CHECK_VULKAN( err, "vkCreateSampler" );

        sampler.magFilter = VK_FILTER_NEAREST;
        sampler.minFilter = VK_FILTER_NEAREST;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		err = vkCreateSampler( device, &sampler, nullptr, pointClamp );
        AE3D_CHECK_VULKAN( err, "vkCreateSampler" );

        sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        err = vkCreateSampler( device, &sampler, nullptr, pointRepeat );
        AE3D_CHECK_VULKAN( err, "vkCreateSampler" );
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
		if (debug::enabled)
		{
			enabledExtensions.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
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

        ++Stats::barrierCalls;
    }
}
