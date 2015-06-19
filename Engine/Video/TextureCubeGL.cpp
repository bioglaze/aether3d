#include <GL/glxw.h>
#include "stb_image.c"
#include <string>
#include <vector>
#include "TextureCube.hpp"
#include "GfxDevice.hpp"
#include "FileSystem.hpp"
#include "System.hpp"

bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp

void ae3d::TextureCube::Load( const FileSystem::FileContentsData& negX, const FileSystem::FileContentsData& posX,
          const FileSystem::FileContentsData& negY, const FileSystem::FileContentsData& posY,
          const FileSystem::FileContentsData& negZ, const FileSystem::FileContentsData& posZ,
          TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps )
{
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;

    if (handle == 0)
    {
        handle = GfxDevice::CreateTextureId();

        if (GfxDevice::HasExtension( "KHR_debug" ))
        {
            glObjectLabel( GL_TEXTURE, handle, (GLsizei)negX.path.size(), negX.path.c_str() );
        }
    }
    
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_CUBE_MAP, handle );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, mipmaps == Mipmaps::Generate ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );

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
                System::Print( "%s failed to load. stb_image's reason: %s", paths[ face ].c_str(), reason.c_str() );
                return;
            }
            
            opaque = (components == 3 || components == 1);
            
            glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
            
            GfxDevice::ErrorCheck( "Cube map creation" );
            stbi_image_free( data );
        }
        else if (isDDS)
        {
            //LoadDDS( fileContents.path.c_str() );
        }
    }
}

