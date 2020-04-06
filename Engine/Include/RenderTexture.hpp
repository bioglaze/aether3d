#pragma once

#include "TextureBase.hpp"

namespace ae3d
{
    /// Render texture. Can be 2D or cube map.
    class RenderTexture : public TextureBase
    {
  public:
        /// Data type.
        enum class DataType
        {
            UByte,
            Float,
            Float16,
            R32G32
        };

        /// Destroys graphics API objects.
        static void DestroyTextures();

        /// \return Data type.
        DataType GetDataType() const { return dataType; }

        /// \param target Target to resolve to.
        void ResolveTo( RenderTexture* target );
        
        /// \param width Width.
        /// \param height Height.
        /// \param dataType Data type.
        /// \param wrap Wrapping mode.
        /// \param filter Filtering mode.
        /// \param debugName Debug name that is visible in graphics debugging tools. If the name is "resolve", the texture can be used for resolving MSAA render targets with resolveTo().
        void Create2D( int width, int height, DataType dataType, TextureWrap wrap, TextureFilter filter, const char* debugName );

        /// \param dimension Dimension.
        /// \param dataType Data type.
        /// \param wrap Wrapping mode.
        /// \param filter Filtering mode.
        /// \param debugName Debug name that is visible in graphics debugging tools.
        void CreateCube( int dimension, DataType dataType, TextureWrap wrap, TextureFilter filter, const char* debugName );

#if RENDERER_VULKAN
        /// \return Multisample count.
        int GetSampleCount() const { return sampleCount; }

        /// \return frame buffer.
        VkFramebuffer GetFrameBuffer() { return frameBuffer; }

        /// \return color view.
        VkImageView GetColorView() { return color.view; }

        /// \return render pass.
        VkRenderPass GetRenderPass() { return renderPass; }

        /// \return Color image.
        VkImage GetColorImage() { return color.image; }

        /// \return Color image layout.
        VkImageLayout GetColorImageLayout() const { return layout; }

        /// \param aLayout New color image layout.
        void SetColorImageLayout( VkImageLayout aLayout );
#endif

  private:

#if RENDERER_VULKAN
        void CreateRenderPass();

        VkFramebuffer frameBuffer = VK_NULL_HANDLE;

        struct FrameBufferAttachment
        {
            VkImage image;
            VkDeviceMemory mem;
            VkImageView view;
        };

		FrameBufferAttachment color = {};
		FrameBufferAttachment depth = {};
        VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        int sampleCount = 1;
        VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#endif
        DataType dataType = DataType::UByte;
    };
}
