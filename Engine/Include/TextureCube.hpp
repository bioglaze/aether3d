#ifndef TEXTURE_CUBE_H
#define TEXTURE_CUBE_H

#include <string>
#if RENDERER_VULKAN
#include <vulkan/vulkan.h>
#endif
#include "TextureBase.hpp"

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }
    
    /// Cube Map texture.
    class TextureCube : public TextureBase
    {
  public:
        /**
          \param negX Negative X axis texture.
          \param posX Positive X axis texture.
          \param negY Negative Y axis texture.
          \param posY Positive Y axis texture.
          \param negZ Negative Z axis texture.
          \param posZ Positive Z axis texture.
          \param wrap Wrapping mode.
          \param filter Filtering mode.
          \param mipmaps Should mipmaps be generated and used.
          \param colorSpace Color space.
         */
        void Load( const FileSystem::FileContentsData& negX, const FileSystem::FileContentsData& posX,
                   const FileSystem::FileContentsData& negY, const FileSystem::FileContentsData& posY,
                   const FileSystem::FileContentsData& negZ, const FileSystem::FileContentsData& posZ,
                   TextureWrap wrap, TextureFilter filter, Mipmaps mipmaps, ColorSpace colorSpace );
        
        /// \return Positive X texture path.
        const std::string& PosX() const { return posXpath; }

        /// \return Negative X texture path.
        const std::string& NegX() const { return negXpath; }

        /// \return Positive Y texture path.
        const std::string& PosY() const { return posYpath; }
        
        /// \return Negative Y texture path.
        const std::string& NegY() const { return negYpath; }

        /// \return Positive Z texture path.
        const std::string& PosZ() const { return posZpath; }
        
        /// \return Negative Z texture path.
        const std::string& NegZ() const { return negZpath; }

#if RENDERER_VULKAN
        VkImageView& GetView() { return view; }
#endif
        /// Destroys all textures. Called internally at exit.
        static void DestroyTextures();
    
    private:
        std::string posXpath, posYpath, negXpath, negYpath, posZpath, negZpath;
#if RENDERER_VULKAN
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
#endif
    };
}

#endif
