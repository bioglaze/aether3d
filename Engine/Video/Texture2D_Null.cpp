#include "Texture2D.hpp"
#include <algorithm>
#include <vector>
#include <sstream>
#include <map>
#include <GL/glxw.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "FileWatcher.hpp"
#include "System.hpp"
#include "FileSystem.hpp"

extern ae3d::FileWatcher fileWatcher;

namespace Texture2DGlobal
{
    ae3d::Texture2D defaultTexture;
    
    std::map< std::string, ae3d::Texture2D > pathToCachedTexture;
#if DEBUG
    std::map< std::string, std::size_t > pathToCachedTextureSizeInBytes;
    
    void PrintMemoryUsage()
    {
        std::size_t total = 0;
        
        for (const auto& path : pathToCachedTextureSizeInBytes)
        {
            ae3d::System::Print("%s: %d B\n", path.first.c_str(), path.second);
            total += path.second;
        }
        
        ae3d::System::Print( "Total texture usage: %d KiB\n", total / 1024 );
    }
#endif
}

void TexReload( const std::string& path )
{
    auto& tex = Texture2DGlobal::pathToCachedTexture[ path ];

    tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetAnisotropy() );
}

namespace
{
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
}

const ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    if (Texture2DGlobal::defaultTexture.GetWidth() == 0)
    {
        Texture2DGlobal::defaultTexture.width = 32;
        Texture2DGlobal::defaultTexture.height = 32;

        Texture2DGlobal::defaultTexture.handle = 0;
    }
    
    return &Texture2DGlobal::defaultTexture;
}

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, float aAnisotropy )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    anisotropy = aAnisotropy;
    
    if (!fileContents.isLoaded)
    {
        return;
    }
    
    const bool isCached = Texture2DGlobal::pathToCachedTexture.find( fileContents.path ) != Texture2DGlobal::pathToCachedTexture.end();

    if (isCached && handle == 0)
    {
        *this = Texture2DGlobal::pathToCachedTexture[ fileContents.path ];
        return;
    }
    
    // First load.
    if (handle == 0)
    {
        handle = 1;
        fileWatcher.AddFile( fileContents.path, TexReload );
    }

    if (HasStbExtension( fileContents.path ))
    {
        LoadSTB( fileContents );
    }

    Texture2DGlobal::pathToCachedTexture[ fileContents.path ] = *this;
#if DEBUG
    Texture2DGlobal::pathToCachedTextureSizeInBytes[ fileContents.path ] = width * height * 4 * (mipmaps == Mipmaps::Generate ? 1.0f : 1.33333f);
    //Texture2DGlobal::PrintMemoryUsage();
#endif
}

void ae3d::Texture2D::LoadFromAtlas( const FileSystem::FileContentsData& atlasTextureData, const FileSystem::FileContentsData& atlasMetaData, const char* textureName, TextureWrap aWrap, TextureFilter aFilter, float aAnisotropy )
{
    Load( atlasTextureData, aWrap, aFilter, mipmaps, aAnisotropy );

    const std::string metaStr = std::string( std::begin( atlasMetaData.data ), std::end( atlasMetaData.data ) );
    std::stringstream metaStream( metaStr );

    if (atlasMetaData.path.find( ".xml" ) == std::string::npos && atlasMetaData.path.find( ".XML" ) == std::string::npos)
    {
        System::Print("Atlas meta data path %s could not be opened!", atlasMetaData.path.c_str());
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
            if (tokens[ t ].find( "Name" ) != std::string::npos)
            {
                if (tokens[ t + 1 ] == textureName)
                {
                    found = true;
                }
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

void ae3d::Texture2D::LoadDDS( const char* path )
{
}

void ae3d::Texture2D::LoadSTB( const FileSystem::FileContentsData& fileContents )
{
    int components;
    unsigned char* data = stbi_load_from_memory( &fileContents.data[ 0 ], static_cast<int>(fileContents.data.size()), &width, &height, &components, 4 );

    if (data == nullptr)
    {
        const std::string reason( stbi_failure_reason() );
        System::Print( "%s failed to load. stb_image's reason: %s", fileContents.path.c_str(), reason.c_str() );
        return;
    }
  
    opaque = (components == 3 || components == 1);

    stbi_image_free( data );
}
