#ifndef VULKAN_UTILS
#define VULKAN_UTILS

#include <vulkan/vulkan.h>
#include "GfxDevice.hpp"

namespace ae3d
{
    void SetImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout, unsigned layerCount, unsigned mipLevel, unsigned mipLevelCount );

    std::uint64_t GetPSOHash( ae3d::VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode,
        ae3d::GfxDevice::DepthFunc depthFunc, ae3d::GfxDevice::CullMode cullMode, ae3d::GfxDevice::FillMode fillMode );

    void CreateInstance( VkInstance* outInstance );
}

namespace debug
{
    extern bool enabled;
    extern bool hasMarker;
    void Setup( VkInstance instance );
    void SetupDevice( VkDevice device );
    void Free( VkInstance instance );
    void SetObjectName( VkDevice device, std::uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name );
    void BeginRegion( VkCommandBuffer cmdbuffer, const char* pMarkerName, float r, float g, float b );
    void EndRegion( VkCommandBuffer cmdBuffer );
}

#endif
