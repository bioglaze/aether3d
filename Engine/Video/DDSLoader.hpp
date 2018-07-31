#ifndef DDSLOADER_H
#define DDSLOADER_H
#include <stdint.h>
#include <vector>

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
    std::vector< unsigned char > imageData;
    int mipMapCount = 0;
};

namespace ae3d
{
	namespace FileSystem
	{
		struct FileContentsData;
	}
}

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
     
     \param fileContents Contents of .dds file.
     \param cubeMapFace Cube map face index 1-6. For 2D textures use 0.
     \param outWidth Stores the width of the texture in pixels.
     \param outHeight Stores the height of the texture in pixels.
     \param outOpaque Stores info about alpha channel.
     \param output Stores information needed to create D3D12 and Metal API objects.
     \return Load result.
     */
    LoadResult Load( const ae3d::FileSystem::FileContentsData& fileContents, int cubeMapFace, int& outWidth, int& outHeight, bool& outOpaque, Output& output );

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
