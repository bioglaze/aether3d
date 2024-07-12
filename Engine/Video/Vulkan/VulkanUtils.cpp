// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <vulkan/vulkan.h>
#include <cstring>
#include <cstdint>
#include <stdio.h>
#include "GfxDevice.hpp"
#include "Macros.hpp"
#include "System.hpp"
#include "Statistics.hpp"
#include "VertexBuffer.hpp"

namespace debug
{
#if DEBUG
    bool enabled = true;
#else
    bool enabled = false;
#endif
    bool hasMarker = false;
	
    PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT;
    VkDebugUtilsMessengerEXT dbgMessenger = VK_NULL_HANDLE;

    static const char* getObjectType( VkObjectType type )
    {
        switch( type )
        {
        case VK_OBJECT_TYPE_QUERY_POOL: return "VK_OBJECT_TYPE_QUERY_POOL";
        case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION: return "VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION";
        case VK_OBJECT_TYPE_SEMAPHORE: return "VK_OBJECT_TYPE_SEMAPHORE";
        case VK_OBJECT_TYPE_SHADER_MODULE: return "VK_OBJECT_TYPE_SHADER_MODULE";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR: return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
        case VK_OBJECT_TYPE_SAMPLER: return "VK_OBJECT_TYPE_SAMPLER";
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT: return "VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT";
        case VK_OBJECT_TYPE_IMAGE: return "VK_OBJECT_TYPE_IMAGE";
        case VK_OBJECT_TYPE_UNKNOWN: return "VK_OBJECT_TYPE_UNKNOWN";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL: return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
        case VK_OBJECT_TYPE_COMMAND_BUFFER: return "VK_OBJECT_TYPE_COMMAND_BUFFER";
        case VK_OBJECT_TYPE_BUFFER: return "VK_OBJECT_TYPE_BUFFER";
        case VK_OBJECT_TYPE_SURFACE_KHR: return "VK_OBJECT_TYPE_SURFACE_KHR";
        case VK_OBJECT_TYPE_INSTANCE: return "VK_OBJECT_TYPE_INSTANCE";
        case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT: return "VK_OBJECT_TYPE_VALIDATION_CACHE_EXT";
        case VK_OBJECT_TYPE_IMAGE_VIEW: return "VK_OBJECT_TYPE_IMAGE_VIEW";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET: return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
        case VK_OBJECT_TYPE_COMMAND_POOL: return "VK_OBJECT_TYPE_COMMAND_POOL";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE: return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
        case VK_OBJECT_TYPE_DISPLAY_KHR: return "VK_OBJECT_TYPE_DISPLAY_KHR";
        case VK_OBJECT_TYPE_BUFFER_VIEW: return "VK_OBJECT_TYPE_BUFFER_VIEW";
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT: return "VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT";
        case VK_OBJECT_TYPE_FRAMEBUFFER: return "VK_OBJECT_TYPE_FRAMEBUFFER";
        case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE: return "VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE";
        case VK_OBJECT_TYPE_PIPELINE_CACHE: return "VK_OBJECT_TYPE_PIPELINE_CACHE";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT: return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
        case VK_OBJECT_TYPE_DEVICE_MEMORY: return "VK_OBJECT_TYPE_DEVICE_MEMORY";
        case VK_OBJECT_TYPE_FENCE: return "VK_OBJECT_TYPE_FENCE";
        case VK_OBJECT_TYPE_QUEUE: return "VK_OBJECT_TYPE_QUEUE";
        case VK_OBJECT_TYPE_DEVICE: return "VK_OBJECT_TYPE_DEVICE";
        case VK_OBJECT_TYPE_RENDER_PASS: return "VK_OBJECT_TYPE_RENDER_PASS";
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR: return "VK_OBJECT_TYPE_DISPLAY_MODE_KHR";
        case VK_OBJECT_TYPE_EVENT: return "VK_OBJECT_TYPE_EVENT";
        case VK_OBJECT_TYPE_PIPELINE: return "VK_OBJECT_TYPE_PIPELINE";
        default:
            return "unhandled type";
        }
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc( VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, VkDebugUtilsMessageTypeFlagsEXT msgType,
                                            const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* /*userData*/ )
    {
        if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            ae3d::System::Print( "ERROR: %s\n", callbackData->pMessage );
        }
        else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
            {
                ae3d::System::Print( "PERF WARNING: %s\n", callbackData->pMessage );
            }
            else
            {
                ae3d::System::Print( "WARNING: %s\n", callbackData->pMessage );
            }
        }
        else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            ae3d::System::Print( "INFO: %s\n", callbackData->pMessage );
        }
        else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            ae3d::System::Print( "VERBOSE: %s\n", callbackData->pMessage );
        }

        if (msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        {
            ae3d::System::Print( "GENERAL: %s\n", callbackData->pMessage );
        }
        
        if( callbackData->objectCount > 0 )
        {
            ae3d::System::Print( "Objects: %u\n", callbackData->objectCount );

            for( uint32_t i = 0; i < callbackData->objectCount; ++i )
            {
                const char* name = callbackData->pObjects[ i ].pObjectName ? callbackData->pObjects[ i ].pObjectName : "unnamed";
                ae3d::System::Print( "Object %u: name: %s, type: %s\n", i, name, getObjectType( callbackData->pObjects[ i ].objectType ) );
            }
        }

        if( !(msgType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) )
        {
#if _MSC_VER && DEBUG
            //__debugbreak();
#endif
            //ae3d::System::Assert( false, "Vulkan error!" );
        }
        
        return VK_FALSE;
    }

    void Setup( VkInstance instance )
    {
        PFN_vkCreateDebugUtilsMessengerEXT dbgCreateDebugUtilsMessenger = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
        CmdBeginDebugUtilsLabelEXT = ( PFN_vkCmdBeginDebugUtilsLabelEXT )vkGetInstanceProcAddr( instance, "vkCmdBeginDebugUtilsLabelEXT" );
        CmdEndDebugUtilsLabelEXT = ( PFN_vkCmdEndDebugUtilsLabelEXT )vkGetInstanceProcAddr( instance, "vkCmdEndDebugUtilsLabelEXT" );

        if (!debug::enabled)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info = {};
        dbg_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbg_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg_messenger_create_info.pfnUserCallback = dbgFunc;
        VkResult err = dbgCreateDebugUtilsMessenger( instance, &dbg_messenger_create_info, nullptr, &dbgMessenger );
        AE3D_CHECK_VULKAN( err, "Unable to create debug report callback" );
    }

    void SetupDevice( VkDevice device )
    {
        setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr( device, "vkSetDebugUtilsObjectNameEXT" );
    }

    void SetObjectName( VkDevice device, std::uint64_t object, VkObjectType objectType, const char* name )
    {
        if (setDebugUtilsObjectNameEXT && hasMarker)
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo = {};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.objectHandle = object;
            nameInfo.pObjectName = name;
            setDebugUtilsObjectNameEXT( device, &nameInfo );
        }
    }

    void BeginRegion( VkCommandBuffer cmdbuffer, const char* pMarkerName, float r, float g, float b )
    {
        if (CmdBeginDebugUtilsLabelEXT && hasMarker)
        {
            VkDebugUtilsLabelEXT label = {};
            label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            label.pNext = nullptr;
            label.pLabelName = pMarkerName;
            label.color[ 0 ] = r;
            label.color[ 1 ] = g;
            label.color[ 2 ] = b;
            label.color[ 3 ] = 1;
            CmdBeginDebugUtilsLabelEXT( cmdbuffer, &label );
        }
    }

    void EndRegion( VkCommandBuffer cmdBuffer )
    {
        if (CmdEndDebugUtilsLabelEXT && hasMarker)
        {
            CmdEndDebugUtilsLabelEXT( cmdBuffer );
        }
    }

    void Free( VkInstance instance )
    {
        if (enabled)
        {
            PFN_vkDestroyDebugUtilsMessengerEXT dbgDestroyDebugUtilsMessenger;
            dbgDestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
            dbgDestroyDebugUtilsMessenger( instance, dbgMessenger, nullptr );
        }
    }
}

namespace ae3d
{
    std::uint64_t GetPSOHash( ae3d::VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode,
        ae3d::GfxDevice::DepthFunc depthFunc, ae3d::GfxDevice::CullMode cullMode, ae3d::GfxDevice::FillMode fillMode, VkRenderPass renderPass, ae3d::GfxDevice::PrimitiveTopology topology )
    {
        std::uint64_t outResult = (std::uint64_t)vertexBuffer.GetVertexFormat();
        outResult += (ptrdiff_t)&shader;
        outResult += (unsigned)blendMode;
        outResult += ((unsigned)depthFunc) * 2;
        outResult += ((unsigned)cullMode) * 4;
        outResult += ((unsigned)fillMode) * 8;
        outResult += ((ptrdiff_t)renderPass) * 16;
        outResult += ((unsigned)topology) * 32;

        return outResult;
    }

    void CreateInstance( VkInstance* outInstance )
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Aether3D";
        appInfo.pEngineName = "Aether3D";
        appInfo.apiVersion = VK_API_VERSION_1_1;
     
        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pNext = nullptr;
        instanceCreateInfo.pApplicationInfo = &appInfo;

        const char* enabledExtensions[] =
        {
            VK_KHR_SURFACE_EXTENSION_NAME,
#if VK_USE_PLATFORM_WIN32_KHR
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#if VK_USE_PLATFORM_XCB_KHR
            VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
#if VK_USE_PLATFORM_ANDROID_KHR
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#endif
#if AE3D_OPENVR
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        };

#if VK_USE_PLATFORM_ANDROID_KHR
        const char* validationLayerNames[] =
        {
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_GOOGLE_unique_objects"
        };
        instanceCreateInfo.enabledLayerCount = 5;
#else
        const char* validationLayerNames[] =
        {
            "VK_LAYER_KHRONOS_validation"
        };

        instanceCreateInfo.enabledLayerCount = debug::enabled ? 1 : 0;
#endif

        if (debug::enabled)
        {
            instanceCreateInfo.ppEnabledLayerNames = validationLayerNames;
        }

#if AE3D_OPENVR
        instanceCreateInfo.enabledExtensionCount = debug::enabled ? 4 : 3;
#else
        instanceCreateInfo.enabledExtensionCount = 4;
#endif
        instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

#if 0
        const VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT, VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
        VkValidationFeaturesEXT features = {};
        features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
        features.enabledValidationFeatureCount = 3;
        features.pEnabledValidationFeatures = enables;

        instanceCreateInfo.pNext = &features;
#endif

        VkResult result = vkCreateInstance( &instanceCreateInfo, nullptr, outInstance );
        AE3D_CHECK_VULKAN( result, "instance" );
    }

    void SetupImageBarrier( VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
                            VkImageLayout newImageLayout, unsigned layerCount, unsigned mipLevel, unsigned mipLevelCount,
                            VkImageMemoryBarrier& outBarrier, VkPipelineStageFlags& outSrcStageFlags, VkPipelineStageFlags& outDstStageFlags )
    {
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

        if (oldImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        }

        if (oldImageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        }

        if (oldImageLayout == VK_IMAGE_LAYOUT_GENERAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }

        if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        }

        if (newImageLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
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
            if (imageMemoryBarrier.srcAccessMask == 0)
            {
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            
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

        VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT)
        {
            destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        
        if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_SHADER_READ_BIT)
        {
            destStageFlags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        {
            destStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }

        if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
        {
            destStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }

        if (imageMemoryBarrier.dstAccessMask == VK_ACCESS_TRANSFER_READ_BIT)
        {
            destStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }

        if (imageMemoryBarrier.srcAccessMask == VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
        {
            srcStageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        if (imageMemoryBarrier.srcAccessMask & VK_ACCESS_TRANSFER_WRITE_BIT)
        {
            srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        if (imageMemoryBarrier.srcAccessMask & VK_ACCESS_TRANSFER_READ_BIT)
        {
            srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        if (imageMemoryBarrier.srcAccessMask & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        {
            srcStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }
        if (imageMemoryBarrier.srcAccessMask & VK_ACCESS_SHADER_READ_BIT)
        {
            srcStageFlags = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        outBarrier = imageMemoryBarrier;
        outSrcStageFlags = srcStageFlags;
        outDstStageFlags = destStageFlags;
    }
    
    void SetImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout, unsigned layerCount, unsigned mipLevel, unsigned mipLevelCount )
    {
        System::Assert( cmdbuffer != VK_NULL_HANDLE, "command buffer not initialized" );

        VkImageMemoryBarrier barrier{};
        VkPipelineStageFlags srcStageFlags{};
        VkPipelineStageFlags destStageFlags{};
        SetupImageBarrier( image, aspectMask, oldImageLayout, newImageLayout, layerCount, mipLevel, mipLevelCount, barrier, srcStageFlags, destStageFlags );
        
        vkCmdPipelineBarrier(
            cmdbuffer,
            srcStageFlags,
            destStageFlags,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier );

        Statistics::IncBarrierCalls();
    }
}
