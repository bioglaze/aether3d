#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include "Texture2D.hpp"
#include "System.hpp"
#include "FileSystem.hpp"

#if RENDERER_METAL
namespace Texture2DGlobal
{
    std::map< std::string, ae3d::Texture2D > hashToCachedTexture;
}
#endif

bool HasStbExtension( const std::string& path )
{
    // Checks for uncompressed formats in texture's file name.
    static const std::string extensions[] =
    {
        ".png", ".PNG", ".jpg", ".JPG", ".tga", ".TGA",
        ".bmp", ".BMP", ".gif", ".GIF"
    };
    
    const bool extensionFound = std::any_of( std::begin( extensions ), std::end( extensions ),
                                            [&]( const std::string& extension ) { return path.find( extension ) != std::string::npos; } );
    
    return extensionFound;
}

void Tokenize( const std::string& str,
              std::vector< std::string >& tokens,
              const std::string& delimiters = " " )
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of( delimiters, 0 );
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of( delimiters, lastPos );
    
    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back( str.substr( lastPos, pos - lastPos ) );
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of( delimiters, pos );
        // Find next "non-delimiter"
        pos = str.find_first_of( delimiters, lastPos );
    }
}

float GetFloatAnisotropy( ae3d::Anisotropy anisotropy )
{
    if (anisotropy == ae3d::Anisotropy::k1)
    {
        return 1;
    }
    if (anisotropy == ae3d::Anisotropy::k2)
    {
        return 2;
    }
    if (anisotropy == ae3d::Anisotropy::k4)
    {
        return 4;
    }
    if (anisotropy == ae3d::Anisotropy::k8)
    {
        return 8;
    }

    ae3d::System::Assert( false, "unhandled anisotropy" );
    return 1;
}

namespace Texture2DGlobal
{
    extern std::map< std::string, ae3d::Texture2D > hashToCachedTexture;
}

namespace ae3d
{
    std::string GetCacheHash( const std::string& path, ae3d::TextureWrap wrap, ae3d::TextureFilter filter, ae3d::Mipmaps mipmaps, ae3d::ColorSpace colorSpace, ae3d::Anisotropy anisotropy )
    {
        return path + std::to_string( static_cast<int>(wrap) ) + std::to_string( static_cast<int>(filter) ) +
            std::to_string( static_cast<int>(mipmaps) ) + std::to_string( static_cast<int>(colorSpace) ) + std::to_string( static_cast< int >(anisotropy) );
    }
}

void TexReload( const std::string& path )
{
    std::string cacheHash = GetCacheHash( path, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    if (Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != std::end( Texture2DGlobal::hashToCachedTexture ))
    {
        auto& tex = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
    }

    cacheHash = GetCacheHash( path, ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::Generate, ae3d::ColorSpace::RGB, ae3d::Anisotropy::k1 );

    if (Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != std::end( Texture2DGlobal::hashToCachedTexture ))
    {
        auto& tex = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
    }

    cacheHash = GetCacheHash( path, ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::None, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    if (Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != std::end( Texture2DGlobal::hashToCachedTexture ))
    {
        auto& tex = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
    }

    cacheHash = GetCacheHash( path, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Linear, ae3d::Mipmaps::None, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    if (Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != std::end( Texture2DGlobal::hashToCachedTexture ))
    {
        auto& tex = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
    }

    cacheHash = GetCacheHash( path, ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::None, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    if (Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != std::end( Texture2DGlobal::hashToCachedTexture ))
    {
        auto& tex = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
    }

    cacheHash = GetCacheHash( path, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Linear, ae3d::Mipmaps::Generate, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    if (Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != std::end( Texture2DGlobal::hashToCachedTexture ))
    {
        auto& tex = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
    }

    cacheHash = GetCacheHash( path, ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Linear, ae3d::Mipmaps::Generate, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    if (Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != std::end( Texture2DGlobal::hashToCachedTexture ))
    {
        auto& tex = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
    }

    cacheHash = GetCacheHash( path, ae3d::TextureWrap::Repeat, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::Generate, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    if (Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != std::end( Texture2DGlobal::hashToCachedTexture ))
    {
        auto& tex = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
    }

    cacheHash = GetCacheHash( path, ae3d::TextureWrap::Clamp, ae3d::TextureFilter::Nearest, ae3d::Mipmaps::Generate, ae3d::ColorSpace::SRGB, ae3d::Anisotropy::k1 );

    if (Texture2DGlobal::hashToCachedTexture.find( cacheHash ) != std::end( Texture2DGlobal::hashToCachedTexture ))
    {
        auto& tex = Texture2DGlobal::hashToCachedTexture[ cacheHash ];
        tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
    }
}

void ae3d::Texture2D::LoadFromAtlas( const FileSystem::FileContentsData& atlasTextureData, const FileSystem::FileContentsData& atlasMetaData, const char* textureName, TextureWrap aWrap, TextureFilter aFilter, ColorSpace aColorSpace, Anisotropy aAnisotropy )
{
    Load( atlasTextureData, aWrap, aFilter, mipmaps, aColorSpace, aAnisotropy );

    const std::string metaStr = std::string( std::begin( atlasMetaData.data ), std::end( atlasMetaData.data ) );
    std::stringstream metaStream( metaStr );

    if (atlasMetaData.path.find( ".xml" ) == std::string::npos && atlasMetaData.path.find( ".XML" ) == std::string::npos)
    {
        System::Print( "Atlas meta data path %s extension is not .xml!", atlasMetaData.path.c_str() );
        return;
    }

    std::string line;

    while (std::getline( metaStream, line ))
    {
        if (line.find( "<Image Name" ) == std::string::npos)
        {
            continue;
        }

        std::vector< std::string > tokens;
        Tokenize( line, tokens, "\"" );
        bool found = false;

        for (std::size_t t = 0; t < tokens.size(); ++t)
        {
            if (tokens[ t ].find( "Name" ) != std::string::npos && tokens[ t + 1 ] == textureName)
            {
                found = true;
            }

            if (!found)
            {
                continue;
            }

            if (tokens[ t ].find( "XPos" ) != std::string::npos)
            {
                scaleOffset.z = std::stoi( tokens[ t + 1 ] ) / static_cast<float>(width);
            }
            else if (tokens[ t ].find( "YPos" ) != std::string::npos)
            {
                scaleOffset.w = std::stoi( tokens[ t + 1 ] ) / static_cast<float>(height);
            }
            else if (tokens[ t ].find( "Width" ) != std::string::npos)
            {
                const int w = std::stoi( tokens[ t + 1 ] );
                scaleOffset.x = 1.0f / (static_cast<float>(width) / static_cast<float>(w));
                width = w;
            }
            else if (tokens[ t ].find( "Height" ) != std::string::npos)
            {
                const int h = std::stoi( tokens[ t + 1 ] );
                scaleOffset.y = 1.0f / (static_cast<float>(height) / static_cast<float>(h));
                height = h;
            }
        }

        if (found)
        {
            return;
        }
    }
}
