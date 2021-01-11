#pragma once

#include "TextureBase.hpp"

namespace DDSLoader
{
    struct Output;
}

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }

    enum class TextureLayout
    {
        General, ShaderRead, ShaderReadWrite, UAVBarrier
    };
        
    /// 2D texture.
    class Texture2D : public TextureBase
    {
    public:
        /// Gets a default texture that is always available after System::LoadBuiltinAssets().
        static Texture2D* GetDefaultTexture();

        /// Gets a default texture UAV (read-write) that is always available after System::LoadBuiltinAssets().
        static Texture2D* GetDefaultTextureUAV();

        /// \param layout Layout.
        void SetLayout( TextureLayout layout );
        
        /// \param width Width in pixels.
        /// \param height height in pixels.
        /// \param debugName Debug name.
        /// \param format Pixel format.
        /// \param imageData Pixel data, can be null.
        void CreateUAV( int width, int height, const char* debugName, DataType format, const void* imageData );
        
        /// \param textureData Texture image data. File format must be dds, png, tga, jpg or bmp.
        /// \param wrap Wrap mode.
        /// \param filter Filter mode.
        /// \param mipmaps Mipmaps. If set to Generate and the image already contains mipmaps, they are used, otherwise mipchain is generated.
        /// \param colorSpace Color space.
        /// \param anisotropy Anisotropy. Value range is 1-16 depending on support. On Metal the value is bucketed into 1, 2, 4, 8 and 16.
        void Load( const FileSystem::FileContentsData& textureData, TextureWrap wrap, TextureFilter filter, Mipmaps mipmaps, ColorSpace colorSpace, Anisotropy anisotropy );
        
        /// \param atlasTextureData Atlas texture image data. File format must be dds, png, tga, jpg or bmp.
        /// \param atlasMetaData Atlas metadata. Format is Ogre/CEGUI. Example atlas tool: Texture Packer.
        /// \param textureName Name of the texture in atlas.
        /// \param wrap Wrap mode.
        /// \param filter Filter mode.
        /// \param colorSpace Color space.
        /// \param anisotropy Anisotropy. Value range is 1-16 depending on support. On Metal the value is bucketed into 1, 2, 4, 8 and 16.
        void LoadFromAtlas( const FileSystem::FileContentsData& atlasTextureData, const FileSystem::FileContentsData& atlasMetaData, const char* textureName, TextureWrap wrap, TextureFilter filter, ColorSpace colorSpace, Anisotropy anisotropy );
        
#if RENDERER_VULKAN
        VkImageView& GetView() { return view; }
        VkImage& GetImage() { return image; }
#endif
        /// \param imageData Raw pixel data
        /// \param width Width in pixels
        /// \param height Height in pixels
        /// \param debugName Null-terminated string of texture's debug name that is visible in graphics debugging tools
        /// \param format Pixel format.
        void LoadFromData( const void* imageData, int width, int height, const char* debugName, DataType format );

        /// Destroys all textures. Called internally at exit.
        static void DestroyTextures();

    private:
        /// \param path Path.
        void LoadDDS( const char* path );
        
        /**
          Loads texture from stb_image.c supported formats.

          \param textureData Texture data.
          */
        void LoadSTB( const FileSystem::FileContentsData& textureData );
#if RENDERER_METAL
        void LoadPVRv2( const char* path );
        void LoadPVRv3( const char* path );
#endif
#if RENDERER_VULKAN
        void CreateVulkanObjects( const DDSLoader::Output& mipChain, VkFormat format );
        void CreateVulkanObjects( void* data, int bytesPerPixel, VkFormat format, VkImageUsageFlags usageFlags );
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;
#endif
    };
}
