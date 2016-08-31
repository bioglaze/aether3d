#ifndef DDSLOADER_H
#define DDSLOADER_H
#include <stdint.h>
#include <vector>
#include "FileSystem.hpp"

#define DDS_MAGIC 0x20534444 ///<  little-endian

//  DDS_header.dwFlags
#define DDSD_CAPS                   0x00000001 
#define DDSD_HEIGHT                 0x00000002 ///< Height in pixels.
#define DDSD_WIDTH                  0x00000004 ///< Width in pixels. 
#define DDSD_PITCH                  0x00000008 ///< Pitch.
#define DDSD_PIXELFORMAT            0x00001000 ///< Pixel format.
#define DDSD_MIPMAPCOUNT            0x00020000 ///< # of mipmaps.
#define DDSD_LINEARSIZE             0x00080000 
#define DDSD_DEPTH                  0x00800000 

//  DDS_header.sPixelFormat.dwFlags
#define DDPF_ALPHAPIXELS            0x00000001 
#define DDPF_FOURCC                 0x00000004 ///< Type of compression.
#define DDPF_INDEXED                0x00000020 
#define DDPF_RGB                    0x00000040 

//  DDS_header.sCaps.dwCaps1
#define DDSCAPS_COMPLEX             0x00000008 
#define DDSCAPS_TEXTURE             0x00001000 
#define DDSCAPS_MIPMAP              0x00400000 

//  DDS_header.sCaps.dwCaps2
#define DDSCAPS2_CUBEMAP            0x00000200 
#define DDSCAPS2_CUBEMAP_POSITIVEX  0x00000400 
#define DDSCAPS2_CUBEMAP_NEGATIVEX  0x00000800 
#define DDSCAPS2_CUBEMAP_POSITIVEY  0x00001000 
#define DDSCAPS2_CUBEMAP_NEGATIVEY  0x00002000 
#define DDSCAPS2_CUBEMAP_POSITIVEZ  0x00004000 
#define DDSCAPS2_CUBEMAP_NEGATIVEZ  0x00008000 
#define DDSCAPS2_VOLUME             0x00200000 

#define D3DFMT_DXT1 827611204 ///< DXT1 compression texture format 
#define D3DFMT_DXT2 0 ///< DXT2 compression texture format, FIXME: find this
#define D3DFMT_DXT3 861165636 ///< DXT3 compression texture format 
#define D3DFMT_DXT4 0 ///< DXT4 compression texture format, FIXME: find this
#define D3DFMT_DXT5 894720068 ///< DXT5 compression texture format 

#define PF_IS_DXT1(pf) \
  ((pf.dwFlags & DDPF_FOURCC) && \
   (pf.dwFourCC == D3DFMT_DXT1))

#define PF_IS_DXT3(pf) \
  ((pf.dwFlags & DDPF_FOURCC) && \
   (pf.dwFourCC == D3DFMT_DXT3))

#define PF_IS_DXT5(pf) \
  ((pf.dwFlags & DDPF_FOURCC) && \
   (pf.dwFourCC == D3DFMT_DXT5))

#define PF_IS_BGRA8(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
   (pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 32) && \
   (pf.dwRBitMask == 0xff0000) && \
   (pf.dwGBitMask == 0xff00) && \
   (pf.dwBBitMask == 0xff) && \
   (pf.dwAlphaBitMask == 0xff000000U))

#define PF_IS_BGR8(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
  !(pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 24) && \
   (pf.dwRBitMask == 0xff0000) && \
   (pf.dwGBitMask == 0xff00) && \
   (pf.dwBBitMask == 0xff))

#define PF_IS_BGR5A1(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
   (pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 16) && \
   (pf.dwRBitMask == 0x00007c00) && \
   (pf.dwGBitMask == 0x000003e0) && \
   (pf.dwBBitMask == 0x0000001f) && \
   (pf.dwAlphaBitMask == 0x00008000))

#define PF_IS_BGR565(pf) \
  ((pf.dwFlags & DDPF_RGB) && \
  !(pf.dwFlags & DDPF_ALPHAPIXELS) && \
   (pf.dwRGBBitCount == 16) && \
   (pf.dwRBitMask == 0x0000f800) && \
   (pf.dwGBitMask == 0x000007e0) && \
   (pf.dwBBitMask == 0x0000001f))

#define PF_IS_INDEX8(pf) \
  ((pf.dwFlags & DDPF_INDEXED) && \
   (pf.dwRGBBitCount == 8))

struct DDSInfo
{
    DDSInfo( bool aIsCompressed, bool aSwap, bool aHasPalette, int aDivSize, int aBlockBytes
#if RENDERER_OPENGL
        , int aInternalFormat, int aExternalFormat, int aType
#endif
    )
        : isCompressed( aIsCompressed )
        , swap( aSwap )
        , hasPalette( aHasPalette )
        , divSize( aDivSize )
        , blockBytes( aBlockBytes )
#if RENDERER_OPENGL
        , internalFormat( aInternalFormat )
        , externalFormat( aExternalFormat )
        , type( aType )
#endif
    {}

    bool isCompressed; ///< Is the file compressed.
    bool swap;
    bool hasPalette; ///< Does the file contain a palette.
    unsigned divSize;
    unsigned blockBytes;
    std::vector< unsigned char > imageData;
    int mipMapCount = 0;
#if RENDERER_OPENGL
    int internalFormat;
    int externalFormat;
    int type;
#endif
};

/// Loads a .dds (DirectDraw Surface)
namespace DDSLoader
{
    /// Load result
    enum class LoadResult { Success, UnknownPixelFormat, FileNotFound };
    /// Format
    enum class Format { Invalid, BC1, BC2, BC3 };

    struct Output
    {
        std::vector< unsigned char > imageData;
        std::vector< std::size_t > dataOffsets; // Mipmap offsets in imageData
        Format format = Format::Invalid;
    };
 
    /**
     Loads a .dds file.
     
     OpenGL renderer:
     Stores the image data into the currently bound texture.
     Texture must be bound before calling this method.

     \param fileContents Contents of .dds file.
     \param cubeMapFace Cube map face index 1-6. For 2D textures use 0.
     \param outWidth Stores the width of the texture in pixels.
     \param outHeight Stores the height of the texture in pixels.
     \param outOpaque Stores info about alpha channel.
     \param outOutput Stores information needed to create D3D12 and Metal API objects. Not used in OpenGL.
     \return Load result.
     */
    LoadResult Load( const ae3d::FileSystem::FileContentsData& fileContents, int cubeMapFace, int& outWidth, int& outHeight, bool& outOpaque, Output& outOutput );

    namespace
    {
        /**
        DDS header structure.
        */
        union DDSHeader
        {
            struct
            {
                uint32_t dwMagic;
                uint32_t dwSize;
                uint32_t dwFlags;
                uint32_t dwHeight;
                uint32_t dwWidth;
                uint32_t dwPitchOrLinearSize;
                uint32_t dwDepth;
                uint32_t dwMipMapCount;
                uint32_t dwReserved1[ 11 ];

                // DDPIXELFORMAT
                struct
                {
                    uint32_t dwSize;
                    uint32_t dwFlags;
                    uint32_t dwFourCC;
                    uint32_t dwRGBBitCount;
                    uint32_t dwRBitMask;
                    uint32_t dwGBitMask;
                    uint32_t dwBBitMask;
                    uint32_t dwAlphaBitMask;
                } sPixelFormat;

                // DDCAPS2
                struct
                {
                    uint32_t dwCaps1;
                    uint32_t dwCaps2;
                    uint32_t dwDDSX;
                    uint32_t dwReserved;
                } sCaps;

                uint32_t dwReserved2;
            } sHeader;

            uint8_t data[ 128 ];
        };
    }
}
#endif
