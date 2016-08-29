#include "Texture2D.hpp"
#include <algorithm>
#include <vector>
#include <sstream>
#include <map>
#include <GL/glxw.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.c"
#include "DDSLoader.hpp"
#include "GfxDevice.hpp"
#include "FileWatcher.hpp"
#include "FileSystem.hpp"
#include "System.hpp"

extern ae3d::FileWatcher fileWatcher;
bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp
void Tokenize( const std::string& str,
              std::vector< std::string >& tokens,
              const std::string& delimiters = " " ); // Defined in TextureCommon.cpp

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

    tex.Load( ae3d::FileSystem::FileContents( path.c_str() ), tex.GetWrap(), tex.GetFilter(), tex.GetMipmaps(), tex.GetColorSpace(), tex.GetAnisotropy() );
}

ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    if (Texture2DGlobal::defaultTexture.GetWidth() == 0)
    {
        Texture2DGlobal::defaultTexture.width = 32;
        Texture2DGlobal::defaultTexture.height = 32;

        Texture2DGlobal::defaultTexture.handle = GfxDevice::CreateTextureId();
    
        glBindTexture( GL_TEXTURE_2D, Texture2DGlobal::defaultTexture.handle );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

        if (GfxDevice::HasExtension( "GL_KHR_debug" ))
        {
            glObjectLabel( GL_TEXTURE, Texture2DGlobal::defaultTexture.handle, (GLsizei)std::string( "default texture 2d" ).size(), "default texture 2d" );
        }

        int data[ 32 * 32 * 4 ] = { 0xFFC0CB };
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, Texture2DGlobal::defaultTexture.width, Texture2DGlobal::defaultTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
        GfxDevice::ErrorCheck( "Load Texture2D" );
    }
    
    return &Texture2DGlobal::defaultTexture;
}

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace, float aAnisotropy )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    anisotropy = aAnisotropy;
    colorSpace = aColorSpace;

    if (!fileContents.isLoaded)
    {
        *this = Texture2DGlobal::defaultTexture;
        return;
    }
    
    const std::string cacheHash = GetCacheHash( fileContents.path, aWrap, aFilter, aMipmaps, aColorSpace, aAnisotropy );
    const bool isCached = Texture2DGlobal::pathToCachedTexture.find( cacheHash ) != Texture2DGlobal::pathToCachedTexture.end();

    if (isCached && handle == 0)
    {
        *this = Texture2DGlobal::pathToCachedTexture[ cacheHash ];
        return;
    }
    
    // First load.
    if (handle == 0)
    {
        handle = GfxDevice::CreateTextureId();
        
        fileWatcher.AddFile( fileContents.path, TexReload );
    }

    GLenum magFilter = GL_NEAREST;
    GLenum minFilter = GL_NEAREST;
    
    if (filter == TextureFilter::Linear)
    {
        minFilter = GL_LINEAR;
        magFilter = GL_LINEAR;
        
        if (aMipmaps == Mipmaps::Generate)
        {
            minFilter = GL_LINEAR_MIPMAP_LINEAR;
        }
    }
    
    glBindTexture( GL_TEXTURE_2D, handle );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter );
    //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (wrap == TextureWrap::Repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (wrap == TextureWrap::Repeat) ? GL_REPEAT : GL_CLAMP_TO_EDGE );

    if (GfxDevice::HasExtension( "GL_KHR_debug" ))
    {
        glObjectLabel( GL_TEXTURE, handle, (GLsizei)fileContents.path.size(), fileContents.path.c_str() );
    }

    if (GfxDevice::HasExtension( "GL_EXT_texture_filter_anisotropic" ) && anisotropy > 1)
    {
        glTexParameterf( GL_TEXTURE_2D, 0x84FE/*GL_TEXTURE_MAX_ANISOTROPY_EXT*/, anisotropy );
    }

    const bool isDDS = fileContents.path.find( ".dds" ) != std::string::npos || fileContents.path.find( ".DDS" ) != std::string::npos;
    
    if (HasStbExtension( fileContents.path ))
    {
        LoadSTB( fileContents );
    }
    else if (isDDS)
    {
        LoadDDS( fileContents.path.c_str() );
    }
    else
    {
        System::Print( "Unhandled texture extension in file %s\n", fileContents.path.c_str() );
    }

    if (mipmaps == Mipmaps::Generate)
    {
        glGenerateMipmap( GL_TEXTURE_2D );
    }

    Texture2DGlobal::pathToCachedTexture[ cacheHash ] = *this;
#if DEBUG
    Texture2DGlobal::pathToCachedTextureSizeInBytes[ fileContents.path ] = static_cast< std::size_t >(width * height * 4 * (mipmaps == Mipmaps::Generate ? 1.0f : 1.33333f));
    //Texture2DGlobal::PrintMemoryUsage();
#endif
    GfxDevice::ErrorCheck( "Load Texture2D" );
}

void ae3d::Texture2D::LoadDDS( const char* path )
{
    DDSLoader::Output unusedOutput;
    const DDSLoader::LoadResult loadResult = DDSLoader::Load( FileSystem::FileContents( path ), 0, width, height, opaque, unusedOutput );

    if (loadResult != DDSLoader::LoadResult::Success)
    {
        ae3d::System::Print( "DDS Loader could not load %s", path );
    }
}

void ae3d::Texture2D::LoadSTB( const FileSystem::FileContentsData& fileContents )
{
    int components;
    unsigned char* data = stbi_load_from_memory( fileContents.data.data(), static_cast<int>(fileContents.data.size()), &width, &height, &components, 4 );

    if (data == nullptr)
    {
        const std::string reason( stbi_failure_reason() );
        System::Print( "%s failed to load. stb_image's reason: %s\n", fileContents.path.c_str(), reason.c_str() );
        return;
    }
  
    opaque = (components == 3 || components == 1);

    // FIXME: Disabled because immutable textures cannot be reloaded. Enable for shipping builds for performance.
    /*if (GfxDevice::HasExtension( "GL_ARB_texture_storage" ))
    {
        glTexStorage2D( GL_TEXTURE_2D, 1, colorSpace == ColorSpace::RGB ? GL_RGBA8 : GL_SRGB8_ALPHA8, width, height );
        glTexSubImage2D( GL_TEXTURE_2D,
                        0,
                        0, 0,
                        width, height,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        data );
    }
    else*/
    {
        glTexImage2D( GL_TEXTURE_2D, 0, colorSpace == ColorSpace::RGB ? GL_RGBA8 : GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
    }

    GfxDevice::ErrorCheck( "Load Texture2D" );
    stbi_image_free( data );
}
