#pragma once

#include "Array.hpp"

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
    enum class Format { Invalid, BC1, BC2, BC3, BC5 };

    struct Output
    {
        Array< unsigned char > imageData;
        Array< int > dataOffsets; // Mipmap offsets in imageData
        Format format = Format::Invalid;
    };
 
    /**
     Loads a .dds file.
     
     \param fileContents Contents of .dds file.
     \param outWidth Stores the width of the texture in pixels.
     \param outHeight Stores the height of the texture in pixels.
     \param outOpaque Stores info about alpha channel.
     \param output Stores information needed to create D3D12 and Metal API objects.
     \return Load result.
     */
    LoadResult Load( const ae3d::FileSystem::FileContentsData& fileContents, int& outWidth, int& outHeight, bool& outOpaque, Output& output );
}
