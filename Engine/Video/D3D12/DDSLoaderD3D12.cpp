#include "DDSLoader.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

#ifndef max
#define max(a,b) (a > b ? a : b)
#endif

DDSInfo loadInfoDXT1 = { true, false, false, 4, 8 };

DDSInfo loadInfoDXT3 = { true, false, false, 4, 16 };

DDSInfo loadInfoDXT5 = { true, false, false, 4, 16 };

DDSInfo loadInfoBGRA8 = { false, false, false, 1, 4 };

DDSInfo loadInfoBGR8 = { false, false, false, 1, 3 };

DDSInfo loadInfoBGR5A1 = { false, true, false, 1, 2 };

DDSInfo loadInfoBGR565 = { false, true, false, 1, 2 };

DDSInfo loadInfoIndex8 = { false, false, true, 1, 1 };

DDSLoader::LoadResult DDSLoader::Load( const ae3d::FileSystem::FileContentsData& fileContents, int cubeMapFace, int& outWidth, int& outHeight, bool& outOpaque )
{
    DDSHeader hdr;
    std::ifstream ifs( fileContents.path, std::ios::binary );

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
        std::cerr << "Error! Texture " << fileContents.path << " doesn't have pixelformat or caps." << std::endl;
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
        std::cerr << "Error! Texture " << fileContents.path << " has unknown pixel format." << std::endl;
        outWidth = 32;
        outHeight = 32;
        outOpaque = true;
        return LoadResult::UnknownPixelFormat;
    }

    unsigned x = xSize;
    unsigned y = ySize;
    std::size_t size;
    li->mipMapCount = (hdr.sHeader.dwFlags & DDSD_MIPMAPCOUNT) ? hdr.sHeader.dwMipMapCount : 1;

    if (li->isCompressed)
    {
        size = max( li->divSize, x ) / li->divSize * max( li->divSize, y ) / li->divSize * li->blockBytes;
        assert( size == hdr.sHeader.dwPitchOrLinearSize );
        assert( hdr.sHeader.dwFlags & DDSD_LINEARSIZE );

        li->imageData.resize( size );

        if (li->imageData.empty())
        {
            std::cerr << "Error loading texture " << fileContents.path << std::endl;
            outWidth = 32;
            outHeight = 32;
            outOpaque = true;
            return LoadResult::FileNotFound;
        }

        for (int ix = 0; ix < li->mipMapCount; ++ix)
        {
            ifs.read( (char*)&li->imageData[ 0 ], size );
            //glCompressedTexImage2D( cubeMapFace > 0 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeMapFace - 1 : GL_TEXTURE_2D, ix, li->internalFormat, x, y, 0, (GLsizei)size, &data[ 0 ] );
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

        ifs.read( (char *)palette, 4 * 256 );

        for (int ix = 0; ix < li->mipMapCount; ++ix)
        {
            ifs.read( (char *)&data[ 0 ], size );

            for (unsigned zz = 0; zz < size; ++zz)
            {
                unpacked[ zz ] = palette[ data[ zz ] ];
            }

            //glPixelStorei( GL_UNPACK_ROW_LENGTH, y );
            //glTexImage2D( cubeMapFace > 0 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeMapFace - 1 : GL_TEXTURE_2D, ix, li->internalFormat, x, y, 0, li->externalFormat, li->type, &unpacked[ 0 ] );
            x = (x + 1) >> 1;
            y = (y + 1) >> 1;
            size = x * y * li->blockBytes;
        }
    }

    return LoadResult::Success;
}
