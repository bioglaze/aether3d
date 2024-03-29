#pragma once

#include <string>
#if RENDERER_METAL
#import <Metal/Metal.h>
#endif
#if RENDERER_D3D12
#include <d3d12.h>
#endif
#include "Vec3.hpp"
#if RENDERER_VULKAN
#include <vulkan/vulkan.h>
#endif

#if RENDERER_D3D12
// TODO: Move inside engine
struct GpuResource
{
    ID3D12Resource* resource = nullptr;
    D3D12_RESOURCE_STATES usageState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
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

    /// Color space.
    enum class ColorSpace
    {
        Linear,
        SRGB
    };

    /// Anisotropy.
    enum class Anisotropy
    {
        k1,
        k2,
        k4,
        k8
    };

    /// Data type.
    enum class DataType
    {
        UByte,
        Float,
        Float16,
        R32G32,
        R32F
    };

    std::string GetCacheHash( const std::string& path, ae3d::TextureWrap wrap, ae3d::TextureFilter filter, ae3d::Mipmaps mipmaps, ae3d::ColorSpace colorSpace, ae3d::Anisotropy anisotropy );

    enum class TextureLayout
    {
        General, ShaderRead, ShaderReadWrite, UAVBarrier
    };

    /// Base class for textures.
    class TextureBase
    {
  public:
        /// \return id.
        unsigned GetID() const { return handle; }

        /// \return True, if the texture is a cube map.
        bool IsCube() const { return isCube; }

        /// \return Path where this texture was loaded from, if it was loaded from a file.
        const std::string& GetPath() const { return path; }

#if RENDERER_VULKAN
        VkSampler GetSampler() const { return sampler; }
#endif
#if RENDERER_METAL
        id<MTLTexture> GetMetalTexture() const { return metalTexture; }
        id<MTLTexture> GetMetalDepthTexture() const { return metalDepthTexture; }
#endif
#if RENDERER_D3D12
        GpuResource* GetGpuResource() { return &gpuResource; }
        GpuResource* GetGpuResourceDepth() { return &gpuResourceDepth; }
        D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() { return dsv; }
        D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV() { return rtv; }
        DXGI_FORMAT GetDXGIFormat() const { return dxgiFormat; }
        int GetMipLevelCount() const { return mipLevelCount; }

        D3D12_CPU_DESCRIPTOR_HANDLE& GetCubeDSV( unsigned cubeMapFace ) { return cubeDsvs[ cubeMapFace ]; }
        D3D12_CPU_DESCRIPTOR_HANDLE& GetCubeRTV( unsigned cubeMapFace ) { return cubeRtvs[ cubeMapFace ]; }
        const D3D12_SHADER_RESOURCE_VIEW_DESC* GetSRVDesc() const { return &srvDesc; }
        const D3D12_DEPTH_STENCIL_VIEW_DESC* GetDSVDesc() const { return &dsvDesc; }
        const D3D12_UNORDERED_ACCESS_VIEW_DESC* GetUAVDesc() const { return &uavDesc; }
        const D3D12_UNORDERED_ACCESS_VIEW_DESC* GetUAVDescDepth() const { return &uavDescDepth; }
#endif
        /// \return Color space.
        ColorSpace GetColorSpace() const { return colorSpace; }
        
        /// \return Anisotropy.
        Anisotropy GetAnisotropy() const { return anisotropy; }
        
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

        /// \return True, if the texture is a RenderTexture.
        bool IsRenderTexture() const { return isRenderTexture; }

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
        /// Scale (tiling) and offset.
        Vec4 scaleOffset{ 1, 1, 0, 0 };
        /// Mipmaps.
        Mipmaps mipmaps = Mipmaps::None;
        /// Mipmap count.
        int mipLevelCount = 1;
        /// Anisotropy.
        Anisotropy anisotropy = Anisotropy::k1;
        /// Color space.
        ColorSpace colorSpace = ColorSpace::Linear;
        /// Is the texture a cube map?
        bool isCube = false;

#if RENDERER_METAL
        id<MTLTexture> metalTexture;
        id<MTLTexture> metalDepthTexture;
#endif
#if RENDERER_D3D12
        GpuResource gpuResource;
        GpuResource gpuResourceDepth;

        D3D12_CPU_DESCRIPTOR_HANDLE uav = {};
        D3D12_CPU_DESCRIPTOR_HANDLE uavDepth = {};
        D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = {};
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = {};
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescDepth = {};

        D3D12_CPU_DESCRIPTOR_HANDLE cubeRtvs[ 6 ] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE cubeDsvs[ 6 ] = {};
        DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		bool createUAV = false;
#endif
        /// Is the texture opaque.
        bool opaque = true;
        /// Is the texture a render texture.
        bool isRenderTexture = false;
        /// Path where this texture was loaded from, if it was loaded from a file.
        std::string path;
#if RENDERER_VULKAN
        VkSampler sampler = VK_NULL_HANDLE;
#endif
    };
}
