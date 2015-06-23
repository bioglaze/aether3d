#include "TextureCube.hpp"
#include <string>
#include <vector>
#include "stb_image.c"
#include "GfxDevice.hpp"
#include "FileSystem.hpp"
#include "System.hpp"

bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp
void Tokenize( const std::string& str,
              std::vector< std::string >& tokens,
              const std::string& delimiters = " " ); // Defined in TextureCommon.cpp

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

    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatPVRTC_RGBA_4BPP
                                                                                          size:width
                                                                                      mipmapped:NO];
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:descriptor];
    
    int firstImageComponents;
    unsigned char* firstImageData = stbi_load_from_memory( datas[ 0 ]->data(), static_cast<int>(datas[ 0 ]->size()), &width, &height, &firstImageComponents, 4 );
    stbi_image_free( firstImageData );

    const NSUInteger bytesPerPixel = 4;
    const NSUInteger bytesPerRow = bytesPerPixel * width;
    const NSUInteger bytesPerImage = bytesPerRow * width;
    
    MTLRegion region = MTLRegionMake2D( 0, 0, width, width );

    for (int face = 0; face < 6; ++face)
    {
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
            
            [metalTexture replaceRegion:region
                       mipmapLevel:0
                             slice:face
                         withBytes:data
                       bytesPerRow:bytesPerRow
                     bytesPerImage:bytesPerImage];

            stbi_image_free( data );
        }
    }
}
