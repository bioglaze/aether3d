// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
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

#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))

#define D3DFMT_DXT1 827611204 ///< DXT1 compression texture format 
#define D3DFMT_DXT3 861165636 ///< DXT3 compression texture format 
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

#define PF_IS_BC4U(pf) \
((pf.dwFlags & DDPF_FOURCC) && \
(pf.dwFourCC == MAKEFOURCC('B', 'C', '4', 'U') ))

#define PF_IS_BC4S(pf) \
((pf.dwFlags & DDPF_FOURCC) && \
(pf.dwFourCC == MAKEFOURCC('B', 'C', '4', 'S') ))

#define PF_IS_BC5U(pf) \
((pf.dwFlags & DDPF_FOURCC) && \
(pf.dwFourCC == MAKEFOURCC('B', 'C', '5', 'U') ))

#define PF_IS_BC5S(pf) \
((pf.dwFlags & DDPF_FOURCC) && \
(pf.dwFourCC == MAKEFOURCC('B', 'C', '5', 'S') ))

#define PF_IS_DX10(pf) \
((pf.dwFlags & DDPF_FOURCC) && \
(pf.dwFourCC == MAKEFOURCC('D', 'X', '1', '0') ))

#define PF_IS_BC5_ATI1(pf) \
((pf.dwFlags & DDPF_FOURCC) && \
(pf.dwFourCC == MAKEFOURCC('A', 'T', 'I', '1') ))

#define PF_IS_BC5_ATI2(pf) \
((pf.dwFlags & DDPF_FOURCC) && \
(pf.dwFourCC == MAKEFOURCC('A', 'T', 'I', '2') ))

unsigned MyMax( unsigned a, unsigned b )
{
    return (a > b ? a : b);
}

struct DDSInfo
{
    DDSInfo( int aDivSize, int aBlockBytes )
    : divSize( aDivSize )
    , blockBytes( aBlockBytes )
    {}
    
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

DDSInfo loadInfoDXT1 = { 4, 8 };
DDSInfo loadInfoDXT3 = { 4, 16 };
DDSInfo loadInfoDXT5 = { 4, 16 };
DDSInfo loadInfoBC4 = { 4, 16 };
DDSInfo loadInfoBC5 = { 4, 16 };
DDSInfo loadInfoBC5_ATI2 = { 4, 16 };

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
  
    if (!(header.sHeader.dwFlags & DDSD_PIXELFORMAT) || !(header.sHeader.dwFlags & DDSD_CAPS) )
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
    else if (PF_IS_BC4U( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoBC4;
        outOpaque = true;
        output.format = DDSLoader::Format::BC4U;
    }
    else if (PF_IS_BC4S( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoBC4;
        outOpaque = true;
        output.format = DDSLoader::Format::BC4S;
    }
    else if (PF_IS_BC5S( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoBC5;
        outOpaque = true;
        output.format = DDSLoader::Format::BC5S;
    }
    else if (PF_IS_BC5U( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoBC5;
        outOpaque = true;
        output.format = DDSLoader::Format::BC5U;
    }
    else if (PF_IS_BC5_ATI1( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoDXT1;
        outOpaque = true;
        output.format = DDSLoader::Format::BC4U;
    }
    else if (PF_IS_BC5_ATI2( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoBC5_ATI2;
        outOpaque = true;
        output.format = DDSLoader::Format::BC5U;
    }
    else if (PF_IS_DX10( header.sHeader.sPixelFormat ))
    {
        li = &loadInfoDXT5;
        outOpaque = true;
        output.format = DDSLoader::Format::BC5U;
    }
    else
    {
        // (pf.dwFlags & DDPF_FOURCC) && pf.dwFourCC
        ae3d::System::Print("DDS loader error: Texture %s has unknown pixelformat. Has FourCC: %d, FourCC: %d\n", fileContents.path.c_str(),
                            (header.sHeader.dwFlags & DDPF_FOURCC), header.sHeader.sPixelFormat.dwFourCC );
        outWidth    = 32;
        outHeight   = 32;
        outOpaque = true;
        return LoadResult::UnknownPixelFormat;
    }

    unsigned x = xSize;
    unsigned y = ySize;
    mipMapCount = (header.sHeader.dwFlags & DDSD_MIPMAPCOUNT) ? header.sHeader.dwMipMapCount : 1;

    std::size_t fileOffset = sizeof( header );

    std::size_t size = MyMax( li->divSize, x ) / li->divSize * MyMax( li->divSize, y ) / li->divSize * li->blockBytes;
    // FIXME: A texture loaded from https://online-converting.com/image/convert2dds/ has dwPitchOrLinearSize set to 0, but otherwise seems to load correctly.
    //ae3d::System::Assert( size == header.sHeader.dwPitchOrLinearSize, "DDSLoader: Wrong pitch or size" );
    //ae3d::System::Assert( (header.sHeader.dwFlags & DDSD_LINEARSIZE) != 0, "DDSLoader: need flag DDSD_LINEARSIZE" );
    
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

    return LoadResult::Success;
}
