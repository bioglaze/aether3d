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
            
            // TODO: Load

            GfxDevice::ErrorCheck( "Cube map creation" );
            stbi_image_free( data );
        }
        else if (isDDS)
        {
            //LoadDDS( fileContents.path.c_str() );
        }
        else
        {
            System::Print( "Cube map face has unsupported texture extension in file: %s\n", paths[ face ].c_str() );
        }
    }

    GfxDevice::ErrorCheck( "Cube map creation" );
}

