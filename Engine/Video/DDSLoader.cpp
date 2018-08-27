#include "DDSLoader.hpp"
#include <string.h>
#include <stdint.h>
#include "System.hpp"
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

unsigned MyMax( unsigned a, unsigned b )
{
    return (a > b ? a : b);
}

struct DDSInfo
{
    DDSInfo( bool aIsCompressed, bool aSwap, bool aHasPalette, int aDivSize, int aBlockBytes
            )
    : isCompressed( aIsCompressed )
    , swap( aSwap )
    , hasPalette( aHasPalette )
    , divSize( aDivSize )
    , blockBytes( aBlockBytes )
    {}
    
    bool isCompressed; ///< Is the file compressed.
    bool swap;
    bool hasPalette; ///< Does the file contain a palette.
    unsigned divSize;
    unsigned blockBytes;
};

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

DDSInfo loadInfoDXT1 = { true, false, false, 4, 8 };

DDSInfo loadInfoDXT3 = { true, false, false, 4, 16 };

DDSInfo loadInfoDXT5 = { true, false, false, 4, 16 };

DDSInfo loadInfoBGRA8 = { false, false, false, 1, 4 };

DDSInfo loadInfoBGR8 = { false, false, false, 1, 3 };

DDSInfo loadInfoBGR5A1 = { false, true, false, 1, 2 };

DDSInfo loadInfoBGR565 = { false, true, false, 1, 2 };

DDSInfo loadInfoIndex8 = { false, false, true, 1, 1 };

DDSLoader::LoadResult DDSLoader::Load( const ae3d::FileSystem::FileContentsData& fileContents, int& outWidth, int& outHeight, bool& outOpaque, Output& output )
{
    DDSHeader header;
    int mipMapCount = 0;

    if (!fileContents.isLoaded)
    {
        outWidth = 512;
        outHeight = 512;
        return LoadResult::FileNotFound;
    }

    if (fileContents.data.size() < sizeof( header ) )
    {
        ae3d::System::Print( "DDS loader error: Texture %s file length is less than DDS header length.\n", fileContents.path.c_str() );
        return LoadResult::FileNotFound;
    }
    
    memcpy( &header, fileContents.data.data(), sizeof( header ) );

    ae3d::System::Assert( header.sHeader.dwMagic == DDS_MAGIC, "DDSLoader: Wrong magic" );
    ae3d::System::Assert( header.sHeader.dwSize == 124, "DDSLoader: Wrong header size" );
  
    if (!(header.sHeader.dwFlags & DDSD_PIXELFORMAT) ||
        !(header.sHeader.dwFlags & DDSD_CAPS) )
    {
        ae3d::System::Print( "DDS loader error: Texture %s doesn't contain pixelformat or caps.\n", fileContents.path.c_str() );
        outWidth    = 32;
        outHeight   = 32;
        outOpaque = true;
        return LoadResult::UnknownPixelFormat;
    }

    const uint32_t xSize = header.sHeader.dwWidth;
    const uint32_t ySize = header.sHeader.dwHeight;
    ae3d::System::Assert( !( xSize & (xSize - 1) ), "DDSLoader: Wrong image width" );
    ae3d::System::Assert( !( ySize & (ySize - 1) ), "DDSLoader: Wrong image height" );

    outWidth  = xSize;
    outHeight = ySize;
    DDSInfo* li = nullptr;

    if (PF_IS_DXT1( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoDXT1;
        outOpaque = true;
        output.format = DDSLoader::Format::BC1;
    }
    else if (PF_IS_DXT3( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoDXT3;
        outOpaque = false;
        output.format = DDSLoader::Format::BC2;
    }
    else if (PF_IS_DXT5( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoDXT5;
        outOpaque = false;
        output.format = DDSLoader::Format::BC3;
    }
    else if (PF_IS_BGRA8( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoBGRA8;
        outOpaque = false;
        output.format = DDSLoader::Format::Invalid;
    }
    else if (PF_IS_BGR8( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoBGR8;
        outOpaque = true;
        output.format = DDSLoader::Format::Invalid;
    }
    else if (PF_IS_BGR5A1( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoBGR5A1;
        outOpaque = false;
        output.format = DDSLoader::Format::Invalid;
    }
    else if (PF_IS_BGR565( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoBGR565;
        outOpaque = true;
        output.format = DDSLoader::Format::Invalid;
    }
    else if (PF_IS_INDEX8( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoIndex8;
        outOpaque = true;
        output.format = DDSLoader::Format::Invalid;
    }
    else
    {
        ae3d::System::Print("DDS loader error: Texture %s has unknown pixelformat.\n", fileContents.path.c_str() );
        outWidth    = 32;
        outHeight   = 32;
        outOpaque = true;
        return LoadResult::UnknownPixelFormat;
    }

    unsigned x = xSize;
    unsigned y = ySize;
    std::size_t size;
    mipMapCount = (header.sHeader.dwFlags & DDSD_MIPMAPCOUNT) ? header.sHeader.dwMipMapCount : 1;

    std::size_t fileOffset = sizeof( header );

    if (li->isCompressed)
    {
        size = MyMax( li->divSize, x ) / li->divSize * MyMax( li->divSize, y ) / li->divSize * li->blockBytes;
        ae3d::System::Assert( size == header.sHeader.dwPitchOrLinearSize, "DDSLoader: Wrong pitch or size" );
        ae3d::System::Assert( (header.sHeader.dwFlags & DDSD_LINEARSIZE) != 0, "DDSLoader, need flag DDSD_LINEARSIZE" );

        if (size == 0)
        {
            ae3d::System::Print("DDS loader error: Texture %s contents are empty.\n", fileContents.path.c_str() );
            outWidth    = 32;
            outHeight   = 32;
            outOpaque = true;
            return LoadResult::FileNotFound;
        }

        output.imageData.Allocate( (int)fileContents.data.size() );
        
        memcpy( output.imageData.elements, fileContents.data.data(), output.imageData.count );
        output.dataOffsets.Allocate( mipMapCount );

        for (int ix = 0; ix < mipMapCount; ++ix)
        {
            output.dataOffsets[ ix ] = (int)fileOffset;

            fileOffset += size;
            x = (x + 1) >> 1;
            y = (y + 1) >> 1;
            size = MyMax( li->divSize, x ) / li->divSize * MyMax( li->divSize, y ) / li->divSize * li->blockBytes;
        }
    }
    else if (li->hasPalette)
    {
        ae3d::System::Assert( (header.sHeader.dwFlags & DDSD_PITCH) != 0, "DDSLoader: need flag DDSD_PITCH" );
        ae3d::System::Assert( header.sHeader.sPixelFormat.dwRGBBitCount == 8, "DDSLoader: Wrong bit count" );
        size = header.sHeader.dwPitchOrLinearSize * ySize;
        ae3d::System::Assert( size == x * y * li->blockBytes, "DDSLoader: Invalid size" );
        unsigned palette[ 256 ];
        memcpy( &palette[ 0 ], fileContents.data.data() + fileOffset, 4 * 256 );
        fileOffset += 4 * 256;

        for (int ix = 0; ix < mipMapCount; ++ix)
        {
            output.dataOffsets[ ix ] = (int)fileOffset;
            fileOffset += size;

            x = (x + 1) >> 1;
            y = (y + 1) >> 1;
            size = x * y * li->blockBytes;
        }
    }
    else
    {
        size = x * y * li->blockBytes;        

        for (int ix = 0; ix < mipMapCount; ++ix)
        {
            output.dataOffsets[ ix ] = (int)fileOffset;
            fileOffset += size;

			x = (x + 1)>>1;
            y = (y + 1)>>1;
            size = x * y * li->blockBytes;
        }
    }

    return LoadResult::Success;
}
