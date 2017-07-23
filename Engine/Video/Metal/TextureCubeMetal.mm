#include "TextureCube.hpp"
#include <string>
#include <vector>
#if TARGET_OS_IPHONE
#define STBI_NEON
#endif
#include "stb_image.c"
#include "DDSLoader.hpp"
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
                              TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace )
{
    if (!negX.isLoaded || !posX.isLoaded || !negY.isLoaded || !posY.isLoaded || !negZ.isLoaded || !posZ.isLoaded)
    {
        ae3d::System::Print( "Cube map contains a texture that's not loaded.\n" );
        return;
    }
    
    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    colorSpace = aColorSpace;
    path = negX.path;

    const std::string paths[] = { posX.path, negX.path, negY.path, posY.path, negZ.path, posZ.path };
    const std::vector< unsigned char >* datas[] = { &posX.data, &negX.data, &negY.data, &posY.data, &negZ.data, &posZ.data };
    const std::vector< const FileSystem::FileContentsData* > fileContents = { &posX, &negX, &negY, &posY, &negZ, &posZ };

    int firstImageComponents;
    const bool isFirstImageDDS = paths[ 0 ].find( ".dds" ) != std::string::npos || paths[ 0 ].find( ".DDS" ) != std::string::npos;
    
    if (isFirstImageDDS)
    {
#if !TARGET_OS_IPHONE
        DDSLoader::Output output;
        DDSLoader::LoadResult ddsLoadResult = DDSLoader::Load( *fileContents[ 0 ], 0, width, height, opaque, output );
        
        if (ddsLoadResult != DDSLoader::LoadResult::Success)
        {
            ae3d::System::Print( "Could not load %s\n", fileContents[ 0 ]->path.c_str() );
            return;
        }
#endif
    }
    else
    {
        unsigned char* firstImageData = stbi_load_from_memory( datas[ 0 ]->data(), static_cast<int>(datas[ 0 ]->size()), &width, &height, &firstImageComponents, 4 );
        stbi_image_free( firstImageData );
    }
    
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:colorSpace == ColorSpace::RGB ? MTLPixelFormatRGBA8Unorm : MTLPixelFormatRGBA8Unorm_sRGB
                                                                                          size:width
                                                                                        mipmapped:(mipmaps == Mipmaps::None ? NO : YES)];
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:descriptor];

    const std::size_t pos = fileContents[ 0 ]->path.find_last_of( "/" );
    
    if (pos != std::string::npos)
    {
        std::string fileName = fileContents[ 0 ]->path.substr( pos );
        metalTexture.label = [NSString stringWithUTF8String:fileName.c_str()];
    }
    else
    {
        metalTexture.label = [NSString stringWithUTF8String:fileContents[ 0 ]->path.c_str()];
    }
    
    const NSUInteger bytesPerPixel = 4;
    NSUInteger bytesPerRow = bytesPerPixel * width;
    const NSUInteger bytesPerImage = bytesPerRow * width;
    
    MTLRegion region = MTLRegionMake2D( 0, 0, width, width );

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
            
            const int bytesPerRow2 = width * 4;

            opaque = (components == 3 || components == 1);
            
            [metalTexture replaceRegion:region
                       mipmapLevel:0
                             slice:face
                         withBytes:data
                       bytesPerRow:bytesPerRow2
                     bytesPerImage:bytesPerImage];

            stbi_image_free( data );
        }
        else if (isDDS)
        {
#if !TARGET_OS_IPHONE
            DDSLoader::Output output;
            DDSLoader::LoadResult ddsLoadResult = DDSLoader::Load( *fileContents[ face ], 0, width, height, opaque, output );

            if (ddsLoadResult != DDSLoader::LoadResult::Success)
            {
                ae3d::System::Print( "Could not load %s\n", fileContents[ face ]->path.c_str() );
                return;
            }

            bytesPerRow = width * 2;
            const int bytesPerRow2 = width * 4;
            
            MTLPixelFormat pixelFormat = MTLPixelFormatRGBA8Unorm;

            if (output.format == DDSLoader::Format::BC1)
            {
                pixelFormat = colorSpace == ColorSpace::RGB ? MTLPixelFormatBC1_RGBA : MTLPixelFormatBC1_RGBA_sRGB;
            }
            else if (output.format == DDSLoader::Format::BC2)
            {
                pixelFormat = colorSpace == ColorSpace::RGB ? MTLPixelFormatBC2_RGBA : MTLPixelFormatBC2_RGBA_sRGB;
                bytesPerRow = width * 4;
            }
            else if (output.format == DDSLoader::Format::BC3)
            {
                pixelFormat = colorSpace == ColorSpace::RGB ? MTLPixelFormatBC3_RGBA : MTLPixelFormatBC3_RGBA_sRGB;
                bytesPerRow = width * 4;
            }

            region = MTLRegionMake2D( 0, 0, width, height );
            [metalTexture replaceRegion:region
                            mipmapLevel:0
                                  slice:face
                              withBytes:&output.imageData[ output.dataOffsets[ 0 ] ]
                            bytesPerRow:bytesPerRow2
                          bytesPerImage:bytesPerImage];

#endif
        }
        else
        {
            System::Print("Cube map texture has unsupported extension %s. Only .bmp, .jpg, .tga, .gif etc. are supported.\n", paths[ face ].c_str() );
        }
    }

    if (mipmaps == Mipmaps::Generate)
    {
        id<MTLCommandQueue> commandQueue = [GfxDevice::GetMetalDevice() newCommandQueue];
        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
        [commandEncoder generateMipmapsForTexture:metalTexture];
        [commandEncoder endEncoding];
        /*[commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
         completionBlock(metalTexture);
         }];*/
        [commandBuffer commit];
    }
}
