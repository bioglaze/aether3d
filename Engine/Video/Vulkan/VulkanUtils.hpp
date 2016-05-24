#ifndef VULKAN_UTILS
#define VULKAN_UTILS

#include <vulkan/vulkan.h>
#include "GfxDevice.hpp"

namespace ae3d
{
    void SetImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout, unsigned layerCount );
    void CreateSamplers( VkDevice device, VkSampler* linearRepeat, VkSampler* linearClamp, VkSampler* pointClamp, VkSampler* pointRepeat );

    unsigned GetPSOHash( ae3d::VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode,
        ae3d::GfxDevice::DepthFunc depthFunc, ae3d::GfxDevice::CullMode cullMode );

    void CreateInstance( VkInstance* outInstance );
}

namespace debug
{
    extern bool enabled;
    extern int validationLayerCount;
    extern const char *validationLayerNames[];
    void Setup( VkInstance instance );
    void SetupDevice( VkDevice device );
    void Free( VkInstance instance );
    void SetObjectName( VkDevice device, std::uint64_t object, VkDebugReportObjectTypeEXT objectType, const char* name );
}

#endif
