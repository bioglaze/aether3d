#ifndef VULKAN_UTILS
#define VULKAN_UTILS

#include <vulkan/vulkan.h>

namespace ae3d
{
	class VertexBuffer;
	class Shader;

	namespace GfxDevice
	{
		enum class DepthFunc;
		enum class BlendMode;
		enum class CullMode;
		enum class FillMode;
		enum class PrimitiveTopology;
	}

    void SetImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout, unsigned layerCount, unsigned mipLevel, unsigned mipLevelCount );

    void SetupImageBarrier( VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout,
                            VkImageLayout newImageLayout, unsigned layerCount, unsigned mipLevel, unsigned mipLevelCount,
                            VkImageMemoryBarrier& outBarrier, VkPipelineStageFlags& outSrcStageFlags, VkPipelineStageFlags& outDstStageFlags );

    std::uint64_t GetPSOHash( ae3d::VertexBuffer& vertexBuffer, ae3d::Shader& shader, ae3d::GfxDevice::BlendMode blendMode,
        ae3d::GfxDevice::DepthFunc depthFunc, ae3d::GfxDevice::CullMode cullMode, ae3d::GfxDevice::FillMode fillMode, VkRenderPass renderPass, ae3d::GfxDevice::PrimitiveTopology topology );

    void CreateInstance( VkInstance* outInstance );
    std::uint32_t GetMemoryType( std::uint32_t typeBits, VkFlags properties );
}

namespace debug
{
    extern bool enabled;
    extern bool hasMarker;
    void Setup( VkInstance instance );
    void SetupDevice( VkDevice device );
    void Free( VkInstance instance );
    void SetObjectName( VkDevice device, std::uint64_t object, VkObjectType objectType, const char* name );
    void BeginRegion( VkCommandBuffer cmdbuffer, const char* pMarkerName, float r, float g, float b );
    void EndRegion( VkCommandBuffer cmdBuffer );
}

#endif
