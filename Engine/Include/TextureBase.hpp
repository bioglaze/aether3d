#ifndef TEXTURE_BASE_H
#define TEXTURE_BASE_H

#include "Vec3.hpp"

namespace ae3d
{
    /// Texture wrap controls behavior when coordinates are outside range 0-1. Repeat should not be used for atlased textures.
    enum class TextureWrap
    {
        Repeat,
        Clamp
    };
    
    /// Filter controls behavior when the texture is smaller or larger than its original size.
    enum class TextureFilter
    {
        Nearest,
        Linear
    };
    
    /// Mipmap usage.
    enum class Mipmaps
    {
        Generate,
        None
    };

    class TextureBase
    {
  public:
        /// \return id.
        unsigned GetID() const { return handle; }
#if AETHER3D_IOS
        id<MTLTexture> GetMetalTexture() const { if (metalTexture == nullptr) { return GetDefaultTexture()->metalTexture; /*System::Print("metalTexture is null\n");*/} return metalTexture; }
#endif
        
        /// \return Width in pixels.
        int GetWidth() const { return width; }
        
        /// \return Width in pixels.
        int GetHeight() const { return height; }
        
        /// \return Wrapping mode
        TextureWrap GetWrap() const { return wrap; }
        
        /// \return Filtering mode
        TextureFilter GetFilter() const { return filter; }
        
        /// \return Mipmap usage.
        Mipmaps GetMipmaps() const { return mipmaps; }
        
        /// \return Scale and offset. x: scale x, y: scale y, z: offset x, w: offset y.
        const Vec4& GetScaleOffset() const { return scaleOffset; }
        
        /// \return True, if the texture does not contain an alpha channel.
        bool IsOpaque() const { return opaque; }

  protected:
        int width = 0;
        int height = 0;
        unsigned handle = 0;
        TextureWrap wrap = TextureWrap::Repeat;
        TextureFilter filter = TextureFilter::Nearest;
        Mipmaps mipmaps = Mipmaps::None;
        Vec4 scaleOffset{ 1, 1, 0, 0 };
        bool opaque = true;
#if AETHER3D_IOS
        id<MTLTexture> metalTexture;  
#endif
    };
}

#endif
