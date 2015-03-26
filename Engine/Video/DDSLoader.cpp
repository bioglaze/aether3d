#include "DDSLoader.hpp"
#include <GL/glxw.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <vector>

#ifndef max
#define max(a,b) (a > b ? a : b)
#endif

#define DDS_MAGIC 0x20534444 ///<  little-endian

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef GL_COMPRESSED_SRGB_S3TC_DXT1_EXT
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#endif

#ifndef COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
#define COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#endif

#ifndef COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
#define COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#endif

#ifndef COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
#define COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif

#ifndef COMPRESSED_SRGB_EXT
#define COMPRESSED_SRGB_EXT 0x8C48
#endif

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
  ((pf.dwFlags & DDPF_ALPHAPIXELS) && \
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

    /**
     Information block.
     */
    struct DDSInfo
    {
        bool isCompressed; ///< Is the file compressed.
        bool swap;
        bool hasPalette; ///< Does the file contain a palette.
        unsigned divSize;
        unsigned blockBytes;
        GLenum internalFormat;
        GLenum externalFormat;
        GLenum type;
    };
}

DDSInfo loadInfoDXT1 = { true, false, false, 4, 8, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA, GL_UNSIGNED_BYTE };

DDSInfo loadInfoDXT3 = { true, false, false, 4, 16, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, GL_UNSIGNED_BYTE };

DDSInfo loadInfoDXT5 = { true, false, false, 4, 16, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, GL_UNSIGNED_BYTE };

DDSInfo loadInfoBGRA8 = { false, false, false, 1, 4, GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE };

DDSInfo loadInfoBGR8 = { false, false, false, 1, 3, GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE };

DDSInfo loadInfoBGR5A1 = { false, true, false, 1, 2, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV };

DDSInfo loadInfoBGR565 = { false, true, false, 1, 2, GL_RGB5, GL_RGB, GL_UNSIGNED_SHORT_5_6_5 };

DDSInfo loadInfoIndex8 = { false, false, true, 1, 1, GL_RGB8, GL_BGRA, GL_UNSIGNED_BYTE };

bool DDSLoader::Load( const char* path, int cubeMapFace, int& outWidth, int& outHeight, bool& outOpaque )
{
    assert( cubeMapFace >= 0 && cubeMapFace < 7 );

    DDSHeader hdr;
    unsigned x = 0;
    unsigned y = 0;
    int mipMapCount = 0;
    std::ifstream ifs( path, std::ios::binary );

    if (!ifs)
    {
        outWidth = 512;
        outHeight = 512;
        return false;
    }

    ifs.read((char*) &hdr, sizeof( hdr ) );
    assert( hdr.sHeader.dwMagic == DDS_MAGIC );
    assert( hdr.sHeader.dwSize == 124 );
  
    if (!(hdr.sHeader.dwFlags & DDSD_PIXELFORMAT) ||
        !(hdr.sHeader.dwFlags & DDSD_CAPS) )
    {
        std::cerr << "Error! Texture " << path << " doesn't have pixelformat or caps." << std::endl;
        outWidth    = 32;
        outHeight   = 32;
        outOpaque = true;
        return false;
    }

    const uint32_t xSize = hdr.sHeader.dwWidth;
    const uint32_t ySize = hdr.sHeader.dwHeight;
    assert( !( xSize & (xSize - 1) ) );
    assert( !( ySize & (ySize - 1) ) );

    outWidth  = xSize;
    outHeight = ySize;
    DDSInfo* li = nullptr;

    if (PF_IS_DXT1( hdr.sHeader.sPixelFormat ))
    {
        li = &loadInfoDXT1;
        outOpaque = true;
    }
    else if (PF_IS_DXT3( hdr.sHeader.sPixelFormat ))
    {
        li = &loadInfoDXT3;
        outOpaque = false;
    }
    else if (PF_IS_DXT5( hdr.sHeader.sPixelFormat ))
    {
        li = &loadInfoDXT5;
        outOpaque = false;
    }
    else if (PF_IS_BGRA8( hdr.sHeader.sPixelFormat ))
    {
        li = &loadInfoBGRA8;
        outOpaque = false;
    }
    else if (PF_IS_BGR8( hdr.sHeader.sPixelFormat ))
    {
        li = &loadInfoBGR8;
        outOpaque = true;
    }
    else if (PF_IS_BGR5A1( hdr.sHeader.sPixelFormat ))
    {
        li = &loadInfoBGR5A1;
        outOpaque = false;
    }
    else if (PF_IS_BGR565( hdr.sHeader.sPixelFormat ))
    {
        li = &loadInfoBGR565;
        outOpaque = true;
    }
    else if (PF_IS_INDEX8( hdr.sHeader.sPixelFormat ))
    {
        li = &loadInfoIndex8;
        outOpaque = true;
    }
    else
    {
        std::cerr << "Error! Texture " << path << " has unknown pixel format." << std::endl;
        outWidth    = 32;
        outHeight   = 32;
        outOpaque = true;
        return false;
    }

    x = xSize;
    y = ySize;
    std::size_t size;
    mipMapCount = (hdr.sHeader.dwFlags & DDSD_MIPMAPCOUNT) ? hdr.sHeader.dwMipMapCount : 1;

    if (mipMapCount == 0)
    {
        glTexParameteri( cubeMapFace > 0 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    }
    
    if (li->isCompressed)
    {
        size = max( li->divSize, x ) / li->divSize * max( li->divSize, y ) / li->divSize * li->blockBytes;
        assert( size == hdr.sHeader.dwPitchOrLinearSize );
        assert( hdr.sHeader.dwFlags & DDSD_LINEARSIZE );

        std::vector< unsigned char > data( size );

        if (data.empty())
        {
            std::cerr << "Error loading texture " << path << std::endl;
            outWidth    = 32;
            outHeight   = 32;
            outOpaque = true;
            return false;
        }

        for (int ix = 0; ix < mipMapCount; ++ix)
        {
            ifs.read( (char*) &data[0], size );
            glCompressedTexImage2D( cubeMapFace > 0 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeMapFace - 1 : GL_TEXTURE_2D, ix, li->internalFormat, x, y, 0, (GLsizei)size, &data[0] );
            x = (x + 1) >> 1;
            y = (y + 1) >> 1;
            size = max( li->divSize, x ) / li->divSize * max( li->divSize, y ) / li->divSize * li->blockBytes;
        }
    }
    else if (li->hasPalette)
    {
        assert( hdr.sHeader.dwFlags & DDSD_PITCH );
        assert( hdr.sHeader.sPixelFormat.dwRGBBitCount == 8 );
        size = hdr.sHeader.dwPitchOrLinearSize * ySize;
        assert( size == x * y * li->blockBytes );
        std::vector< unsigned char > data( size );
        unsigned palette[ 256 ];
        std::vector< unsigned > unpacked( size * 4 );

        ifs.read( (char *) palette, 4 * 256 );

        for (int ix = 0; ix < mipMapCount; ++ix)
        {
            ifs.read( (char *) &data[ 0 ], size );

            for (unsigned zz = 0; zz < size; ++zz)
            {
                unpacked[ zz ] = palette[ data[ zz ] ];
            }

            glPixelStorei( GL_UNPACK_ROW_LENGTH, y );
            glTexImage2D( cubeMapFace > 0 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeMapFace - 1 : GL_TEXTURE_2D, ix, li->internalFormat, x, y, 0, li->externalFormat, li->type, &unpacked[ 0 ] );
            x = (x + 1) >> 1;
            y = (y + 1) >> 1;
            size = x * y * li->blockBytes;
        }
    }
    else
    {
        if (li->swap)
        {
            glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_TRUE );
        }

        size = x * y * li->blockBytes;        
        std::vector< unsigned char > data( size );

        for (int ix = 0; ix < mipMapCount; ++ix)
        {
            ifs.read( (char*) &data[ 0 ], size );            
            glPixelStorei( GL_UNPACK_ROW_LENGTH, y );
            glTexImage2D( cubeMapFace > 0 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeMapFace - 1 : GL_TEXTURE_2D, ix, li->internalFormat, x, y, 0, li->externalFormat, li->type, &data[ 0 ] );
            x = (x + 1)>>1;
            y = (y + 1)>>1;
            size = x * y * li->blockBytes;
        }
    
        glPixelStorei( GL_UNPACK_SWAP_BYTES, GL_FALSE );
    }

    glTexParameteri( cubeMapFace > 0 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipMapCount - 1 );
    return true;
}
