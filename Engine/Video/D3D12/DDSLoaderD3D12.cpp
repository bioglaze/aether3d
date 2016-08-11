#include "DDSLoader.hpp"
#include <cassert>
#include <fstream>
#include <iostream>

namespace
{
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
    };
}

DDSInfo loadInfoDXT1 = { true, false, false, 4, 8 };

DDSInfo loadInfoDXT3 = { true, false, false, 4, 16 };

DDSInfo loadInfoDXT5 = { true, false, false, 4, 16 };

DDSInfo loadInfoBGRA8 = { false, false, false, 1, 4 };

DDSInfo loadInfoBGR8 = { false, false, false, 1, 3 };

DDSInfo loadInfoBGR5A1 = { false, true, false, 1, 2 };

DDSInfo loadInfoBGR565 = { false, true, false, 1, 2 };

DDSInfo loadInfoIndex8 = { false, false, true, 1, 1 };

DDSLoader::LoadResult DDSLoader::Load( const char* path, int cubeMapFace, int& outWidth, int& outHeight, bool& outOpaque )
{
    DDSHeader hdr;
    int mipMapCount = 0;
    std::ifstream ifs( path, std::ios::binary );

    if (!ifs)
    {
        outWidth = 512;
        outHeight = 512;
        return LoadResult::FileNotFound;
    }

    ifs.read( (char*)&hdr, sizeof( hdr ) );
    assert( hdr.sHeader.dwMagic == DDS_MAGIC );
    assert( hdr.sHeader.dwSize == 124 );

    if (!(hdr.sHeader.dwFlags & DDSD_PIXELFORMAT) ||
        !(hdr.sHeader.dwFlags & DDSD_CAPS))
    {
        std::cerr << "Error! Texture " << path << " doesn't have pixelformat or caps." << std::endl;
        outWidth = 32;
        outHeight = 32;
        outOpaque = true;
        return LoadResult::UnknownPixelFormat;
    }

    const uint32_t xSize = hdr.sHeader.dwWidth;
    const uint32_t ySize = hdr.sHeader.dwHeight;
    assert( !(xSize & (xSize - 1)) );
    assert( !(ySize & (ySize - 1)) );

    outWidth = xSize;
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
        outWidth = 32;
        outHeight = 32;
        outOpaque = true;
        return LoadResult::UnknownPixelFormat;
    }

    unsigned x = xSize;
    unsigned y = ySize;
    std::size_t size;
    mipMapCount = (hdr.sHeader.dwFlags & DDSD_MIPMAPCOUNT) ? hdr.sHeader.dwMipMapCount : 1;

}
