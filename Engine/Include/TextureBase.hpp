#ifndef TEXTURE_BASE_H
#define TEXTURE_BASE_H

#if AETHER3D_IOS
#import <Metal/Metal.h>
#endif
#if AETHER3D_D3D12
#include <d3d12.h>
#endif
#include "Vec3.hpp"

#if AETHER3D_D3D12
// TODO: Move inside engine
struct GpuResource
{
    ID3D12Resource* resource = nullptr;
    D3D12_RESOURCE_STATES usageState;
    D3D12_RESOURCE_STATES transitioningState;
    D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = 0;
};
#endif

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

    /// Base class for textures.
    class TextureBase
    {
  public:
        /// \return id.
        unsigned GetID() const { return handle; }
#if AETHER3D_IOS
        id<MTLTexture> GetMetalTexture() const { return metalTexture; }
#endif
        /// \return Anisotropy.
        float GetAnisotropy() const { return anisotropy; }
        
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
        /// Width in pixels.
        int width = 0;
        /// Height in pixels.
        int height = 0;
        /// Graphics API handle.
        unsigned handle = 0;
        /// Wrapping controls how coordinates outside 0-1 are interpreted.
        TextureWrap wrap = TextureWrap::Repeat;
        /// Filtering mode.
        TextureFilter filter = TextureFilter::Nearest;
        /// Mipmaps.
        Mipmaps mipmaps = Mipmaps::None;
        /// Scale (tiling) and offset.
        Vec4 scaleOffset{ 1, 1, 0, 0 };
        /// Is the texture opaque.
        bool opaque = true;
        /// Anisotropy.
        float anisotropy = 1;
#if AETHER3D_IOS
        id<MTLTexture> metalTexture;  
#endif
#if AETHER3D_D3D12
        GpuResource gpuResource;
        D3D12_CPU_DESCRIPTOR_HANDLE srv;
#endif
    };
}

#endif
