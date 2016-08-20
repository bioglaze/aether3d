#include <GL/glxw.h>
#include <vector>
#include "stb_image.c"
#include "TextureCube.hpp"
#include "DDSLoader.hpp"
#include "GfxDevice.hpp"
#include "FileSystem.hpp"
#include "FileWatcher.hpp"
#include "System.hpp"

extern ae3d::FileWatcher fileWatcher;
bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp

namespace TextureCubeGlobal
{
    std::vector< ae3d::TextureCube > cachedTextures;
}

void CubeReload( const std::string& path )
{
    for (auto& texture : TextureCubeGlobal::cachedTextures)
    {
        if (texture.PosX() == path || texture.PosY() == path || texture.PosZ() == path ||
            texture.NegX() == path || texture.NegY() == path || texture.NegZ() == path)
        {
            ae3d::System::Print("Reloading cube map\n");
            texture.Load( ae3d::FileSystem::FileContents( texture.NegX().c_str() ),
                          ae3d::FileSystem::FileContents( texture.PosX().c_str() ),
                          ae3d::FileSystem::FileContents( texture.NegY().c_str() ),
                          ae3d::FileSystem::FileContents( texture.PosY().c_str() ),
                          ae3d::FileSystem::FileContents( texture.NegZ().c_str() ),
                          ae3d::FileSystem::FileContents( texture.PosZ().c_str() ),
                          texture.GetWrap(), texture.GetFilter(), texture.GetMipmaps(), texture.GetColorSpace() );
        }
    }
}

void ae3d::TextureCube::Load( const FileSystem::FileContentsData& negX, const FileSystem::FileContentsData& posX,
          const FileSystem::FileContentsData& negY, const FileSystem::FileContentsData& posY,
          const FileSystem::FileContentsData& negZ, const FileSystem::FileContentsData& posZ,
          TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace )
{
    const bool isCached = [&]()
    {
        for (auto& texture : TextureCubeGlobal::cachedTextures)
        {
            if (texture.PosX() == posX.path && texture.PosY() == posY.path && texture.PosZ() == posZ.path &&
                texture.NegX() == negX.path && texture.NegY() == negY.path && texture.NegZ() == negZ.path)
            {
                return true;
            }
        }

        return false;
    }();
    
    if (isCached && handle == 0)
    {
        for (auto& texture : TextureCubeGlobal::cachedTextures)
        {
            if (texture.PosX() == posX.path && texture.PosY() == posY.path && texture.PosZ() == posZ.path &&
                texture.NegX() == negX.path && texture.NegY() == negY.path && texture.NegZ() == negZ.path)
            {
                *this = texture;
                return;
            }
        }
    }

    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    colorSpace = aColorSpace;
    
    if (handle == 0)
    {
        handle = GfxDevice::CreateTextureId();
        
        fileWatcher.AddFile( negX.path, CubeReload );
        fileWatcher.AddFile( posX.path, CubeReload );
        fileWatcher.AddFile( negY.path, CubeReload );
        fileWatcher.AddFile( posY.path, CubeReload );
        fileWatcher.AddFile( negZ.path, CubeReload );
        fileWatcher.AddFile( posZ.path, CubeReload );
        
        posXpath = posX.path;
        posYpath = posY.path;
        posZpath = posZ.path;
        negXpath = negX.path;
        negYpath = negY.path;
        negZpath = negZ.path;
    }
    
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_CUBE_MAP, handle );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, mipmaps == Mipmaps::Generate ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );

    if (GfxDevice::HasExtension( "GL_KHR_debug" ))
    {
        glObjectLabel( GL_TEXTURE, handle, (GLsizei)negX.path.size(), negX.path.c_str() );
    }

    const std::string paths[] = { posX.path, negX.path, negY.path, posY.path, negZ.path, posZ.path };
    const std::vector< unsigned char >* datas[] = { &posX.data, &negX.data, &negY.data, &posY.data, &negZ.data, &posZ.data };

    for (int face = 0; face < 6; ++face)
    {
        const bool isDDS = paths[ face ].find( ".dds" ) != std::string::npos || paths[ face ].find( ".DDS" ) != std::string::npos;
        
        if (HasStbExtension( paths[ face ] ))
        {
            int components;
            unsigned char* data = stbi_load_from_memory( datas[ face ]->data(), static_cast<int>(datas[ face ]->size()), &width, &height, &components, 4 );
            
            if (data == nullptr)
            {
                const std::string reason( stbi_failure_reason() );
                System::Print( "%s failed to load. stb_image's reason: %s\n", paths[ face ].c_str(), reason.c_str() );
                return;
            }
            
            opaque = (components == 3 || components == 1);
            
            glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, colorSpace == ColorSpace::RGB ? GL_RGBA8 : GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );

            GfxDevice::ErrorCheck( "Cube map creation" );
            stbi_image_free( data );
        }
        else if (isDDS)
        {
            const DDSLoader::LoadResult result = DDSLoader::Load( FileSystem::FileContents( paths[ face ].c_str() ), face + 1, width, height, opaque );
            
            if (result != DDSLoader::LoadResult::Success)
            {
                System::Print( "Could not load cube map face %d: %s\n", face + 1, paths[ face ].c_str() );
            }
        }
    }

    if (!isCached)
    {
        TextureCubeGlobal::cachedTextures.push_back( *this );
    }
    
    GfxDevice::ErrorCheck( "Cube map creation" );
}

