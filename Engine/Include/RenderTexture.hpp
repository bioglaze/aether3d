#pragma once

#include "TextureBase.hpp"

namespace ae3d
{
    /// Render texture. Can be 2D or cube map.
    class RenderTexture : public TextureBase
    {
  public:
        /// Is the texture readable/writable in a shader?
        enum class UavFlag { Disabled, Enabled, EnabledAlsoDepth };

        /// Destroys graphics API objects.
        static void DestroyTextures();

        /// \return Data type.
        DataType GetDataType() const { return dataType; }

        /// \return True, if the render texture has been created with Create2D or CreateCube.
        bool IsCreated() const noexcept { return isCreated; }

        /// \param target Target to resolve to.
        void ResolveTo( RenderTexture* target );
        
        /// \param width Width.
        /// \param height Height.
        /// \param dataType Data type.
        /// \param wrap Wrapping mode.
        /// \param filter Filtering mode.
        /// \param debugName Debug name that is visible in graphics debugging tools.
        /// \param isMultisampled True, if the texture is multisampled.
        void Create2D( int width, int height, DataType dataType, TextureWrap wrap, TextureFilter filter, const char* debugName, bool isMultisampled, UavFlag uavFlag );

        /// \param dimension Dimension.
        /// \param dataType Data type.
        /// \param wrap Wrapping mode.
        /// \param filter Filtering mode.
        /// \param debugName Debug name that is visible in graphics debugging tools.
        void CreateCube( int dimension, DataType dataType, TextureWrap wrap, TextureFilter filter, const char* debugName );

        /// Slow. Enables reading pixels with Map()/Unmap(). Use only for special stuff like picking buffer in Editor.
        /// NOTE: Not all GPUs support this! This was tested successfully on AMD and failed on NVIDIA.
        /// \param debugName Debug name that is visible in graphics debugging tools.
        void MakeCpuReadable( const char* debugName );
        
        /// \param nam New name.
        void SetName( const std::string& nam ) { name = nam; }
        
        /// \return Name.
        const char* GetName() const { return name.c_str(); };
        
        /// Must call MakeCpuReadable() once before mapping.
        /// \return Pixel data for reading. Call Unmap when done reading it.
        void* Map();

        void Unmap();

        /// \return Multisample count.
        int GetSampleCount() const { return sampleCount; }

#if RENDERER_VULKAN

        /// \return frame buffer.
        VkFramebuffer GetFrameBuffer() { return frameBuffer; }

        /// \return frame buffer for a cube map face.
        VkFramebuffer GetFrameBufferFace( unsigned face ) { return frameBufferFaces[ face ]; }

        /// \return color view.
        VkImageView GetColorView() { return color.view; }

        /// \return render pass.
        VkRenderPass GetRenderPass() { return renderPass; }

        /// \return Color image.
        VkImage GetColorImage() { return color.image; }

        /// \return Depth image.
        VkImage GetDepthImage() { return depth.image; }

        /// \return Color image layout.
        VkImageLayout GetColorImageLayout() const { return color.layout; }

        /// \return Depth image layout.
        VkImageLayout GetDepthImageLayout() const { return depth.layout; }

        /// \param aLayout New depth image layout.
        void SetDepthImageLayout( VkImageLayout aLayout, VkCommandBuffer cmdBuffer );

        /// \param aLayout New color image layout.
        void SetColorImageLayout( VkImageLayout aLayout, VkCommandBuffer cmdBuffer );
#endif

  private:

#if RENDERER_VULKAN
        void CreateRenderPass();

        VkFramebuffer frameBuffer = VK_NULL_HANDLE;
        VkFramebuffer frameBufferFaces[ 6 ];
        
        struct FrameBufferAttachment
        {
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory mem;
            VkImageView views[ 6 ]; // 2D views into cube faces.
            VkImageView view; // 2D or cube view depending on texture type.
            VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        };

    public:
		FrameBufferAttachment color = {};
    private:
		FrameBufferAttachment depth = {};
        VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        VkBuffer pixelBuffer = VK_NULL_HANDLE;
        VkDeviceMemory pixelBufferMemory;
#endif
        int sampleCount = 1;
        DataType dataType = DataType::UByte;
        UavFlag uavFlag = UavFlag::Disabled;
        std::string name;
        bool isCreated = false;
    };
}
