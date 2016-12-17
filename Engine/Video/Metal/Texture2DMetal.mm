#include "Texture2D.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#if TARGET_OS_IPHONE
#define STBI_NEON
#endif
#include "stb_image.c"
#include "DDSLoader.hpp"
#include "FileSystem.hpp"
#include "System.hpp"
#include "GfxDevice.hpp"

#define MYMAX(x, y) ((x) > (y) ? (x) : (y))

bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp

namespace PVRType
{
    enum Enum
    {
        kPVRTextureFlagTypePVRTC_2 = 24,
        kPVRTextureFlagTypePVRTC_4
    };
}

struct PVRv2Header
{
    uint32_t headerLength;
    uint32_t height;
    uint32_t width;
    uint32_t mipmapCount;
    uint32_t flags;
    uint32_t dataLength;
    uint32_t bitsPerPixel;
    uint32_t redBitmask;
    uint32_t greenBitmask;
    uint32_t blueBitmask;
    uint32_t alphaBitmask;
    uint32_t pvrTag;
    uint32_t surfaceCount;
};

struct PVRv3Header
{
    uint32_t version;
    uint32_t flags;
    uint64_t pixelFormat;
    uint32_t colorSpace;
    uint32_t channelType;
    uint32_t height;
    uint32_t width;
    uint32_t depth;
    uint32_t surfaceCount;
    uint32_t faceCount;
    uint32_t mipmapCount;
    uint32_t metadataLength;
};

namespace
{
    ae3d::Texture2D defaultTexture;
}

ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    if (defaultTexture.GetWidth() == 0 && GfxDevice::GetMetalDevice() != nullptr)
    {
        defaultTexture.width = 128;
        defaultTexture.height = 128;
        defaultTexture.opaque = false;
        
        MTLTextureDescriptor* textureDescriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                           width:defaultTexture.width
                                                          height:defaultTexture.height
                                                       mipmapped:NO];
        defaultTexture.metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];
        defaultTexture.metalTexture.label = @"Default texture";

        const int components = 4;
        int data[ 128 * 128 * components ] = { 0xFFC0CB };

        MTLRegion region = MTLRegionMake2D(0, 0, defaultTexture.width, defaultTexture.height);
        const int bytesPerRow = components * defaultTexture.width;
        [defaultTexture.metalTexture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerRow];
    }

    return &defaultTexture;
}

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace, float aAnisotropy )
{
    if (!fileContents.isLoaded)
    {
        return;
    }

    filter = aFilter;
    wrap = aWrap;
    mipmaps = aMipmaps;
    colorSpace = aColorSpace;
    anisotropy = aAnisotropy;
    path = fileContents.path;

    /*const bool isCached = Texture2DGlobal::pathToCachedTexture.find( fileContents.path ) != Texture2DGlobal::pathToCachedTexture.end();
    
    if (isCached && handle == 0)
    {
        *this = Texture2DGlobal::pathToCachedTexture[ fileContents.path ];
        return;
    }*/
    
    const bool isPVR = fileContents.path.find( ".pvr" ) != std::string::npos || fileContents.path.find( ".PVR" ) != std::string::npos;
    const bool isDDS = fileContents.path.find( ".dds" ) != std::string::npos || fileContents.path.find( ".dds" ) != std::string::npos;
    
    if (HasStbExtension( fileContents.path ))
    {
        LoadSTB( fileContents );
    }
    else if (isPVR)
    {
        NSString* pvrtcNSString = [NSString stringWithUTF8String: fileContents.path.c_str()];
        NSData* fileData = [NSData dataWithContentsOfFile:pvrtcNSString];
        PVRv2Header* header = (PVRv2Header*) [fileData bytes];
        uint32_t version = CFSwapInt32LittleToHost(header->headerLength);

        const uint32_t PvrV3Magic = 0x03525650;
        
        if (version == PvrV3Magic)
        {
            LoadPVRv3( fileContents.path.c_str() );
        }
        else
        {
            LoadPVRv2( fileContents.path.c_str() );
        }
    }
    else if (isDDS)
    {
#if !TARGET_OS_IPHONE
        DDSLoader::Output output;
        DDSLoader::LoadResult ddsLoadResult = DDSLoader::Load( fileContents, 0, width, height, opaque, output );
        
        if (ddsLoadResult != DDSLoader::LoadResult::Success)
        {
            ae3d::System::Print( "Could not load %s\n", fileContents.path.c_str() );
            return;
        }
        
        int bytesPerRow = width * 2;
        
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

        MTLTextureDescriptor* textureDescriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                           width:width
                                                          height:height
                                                       mipmapped:NO];//(mipmaps == Mipmaps::None ? NO : YES)];
        metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];
        
        std::size_t pos = fileContents.path.find_last_of( "/" );
        if (pos != std::string::npos)
        {
            std::string fileName = fileContents.path.substr( pos );
            metalTexture.label = [NSString stringWithUTF8String:fileName.c_str()];
        }
        else
        {
            metalTexture.label = [NSString stringWithUTF8String:fileContents.path.c_str()];
        }
        
        
        MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
        [metalTexture replaceRegion:region mipmapLevel:0 withBytes:&output.imageData[ output.dataOffsets[ 0 ] ] bytesPerRow:bytesPerRow];
#endif
    }
    else
    {
        ae3d::System::Print( "Unhandled texture extension in %s\n", fileContents.path.c_str() );
    }
}

void ae3d::Texture2D::LoadSTB( const FileSystem::FileContentsData& fileContents )
{
    int components;
    unsigned char* data = stbi_load_from_memory( &fileContents.data[ 0 ], static_cast<int>(fileContents.data.size()), &width, &height, &components, 4 );
    
    if (data == nullptr)
    {
        const std::string reason( stbi_failure_reason() );
        System::Print( "%s failed to load. stb_image's reason: %s\n", fileContents.path.c_str(), reason.c_str() );
        return;
    }

    opaque = (components == 3 || components == 1);

    MTLTextureDescriptor* textureDescriptor =
    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:colorSpace == ColorSpace::RGB ? MTLPixelFormatRGBA8Unorm : MTLPixelFormatRGBA8Unorm_sRGB
                                                       width:width
                                                      height:height
                                                   mipmapped:(mipmaps == Mipmaps::None ? NO : YES)];
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];
    
    std::size_t pos = fileContents.path.find_last_of( "/" );
    if (pos != std::string::npos)
    {
        std::string fileName = fileContents.path.substr( pos );
        metalTexture.label = [NSString stringWithUTF8String:fileName.c_str()];
    }
    else
    {
        metalTexture.label = [NSString stringWithUTF8String:fileContents.path.c_str()];
    }
    
    const int bytesPerRow = width * 4;

    MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerRow];

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

    stbi_image_free( data );
}

void ae3d::Texture2D::LoadPVRv2( const char* path )
{
#if !TARGET_OS_IPHONE
    System::Assert( false, "PVR2 loading only supported on iOS");
    return;
#else
    NSString* pvrtcNSString = [NSString stringWithUTF8String: path];
    NSData* fileData = [NSData dataWithContentsOfFile:pvrtcNSString];
    PVRv2Header* header = (PVRv2Header*) [fileData bytes];
    uint32_t pvrTag = CFSwapInt32LittleToHost(header->pvrTag);

    const char gPVRTexIdentifier[] = "PVR!";
    
    if (gPVRTexIdentifier[0] != ((pvrTag >>  0) & 0xff) ||
        gPVRTexIdentifier[1] != ((pvrTag >>  8) & 0xff) ||
        gPVRTexIdentifier[2] != ((pvrTag >> 16) & 0xff) ||
        gPVRTexIdentifier[3] != ((pvrTag >> 24) & 0xff))
    {
        NSLog( @"PVRTC header is corrupted: %@\n", pvrtcNSString );
        return;
    }
    
    uint32_t flags = CFSwapInt32LittleToHost( header->flags );
    const uint32_t PVR_TEXTURE_FLAG_TYPE_MASK = 0xFF;
    uint32_t formatFlags = flags & PVR_TEXTURE_FLAG_TYPE_MASK;
    
    if (formatFlags == PVRType::kPVRTextureFlagTypePVRTC_4 ||
        formatFlags == PVRType::kPVRTextureFlagTypePVRTC_2)
    {
        uint8_t* bytes = (uint8_t*)([fileData bytes]) + sizeof( PVRv2Header );
        NSMutableArray* imageData = [[NSMutableArray alloc] initWithCapacity:10];
        [imageData removeAllObjects];
        
        opaque = !CFSwapInt32LittleToHost( header->alphaBitmask );
        
        uint32_t dataLength = CFSwapInt32LittleToHost( header->dataLength );
        uint32_t dataOffset = 0;
        width = CFSwapInt32LittleToHost( header->width );
        height = CFSwapInt32LittleToHost( header->height );
        
        while (dataOffset < dataLength)
        {
            uint32_t blockSize;
            uint32_t widthBlocks = 0;
            uint32_t heightBlocks = 0;
            uint32_t bpp = 0;
            
            if (formatFlags == PVRType::kPVRTextureFlagTypePVRTC_4)
            {
                blockSize = 4 * 4; // Pixel by pixel block size for 4bpp
                widthBlocks = width / 4;
                heightBlocks = height / 4;
                bpp = 4;
            }
            else
            {
                blockSize = 8 * 4; // Pixel by pixel block size for 2bpp
                widthBlocks = width / 8;
                heightBlocks = height / 4;
                bpp = 2;
            }
            
            // Clamp to minimum number of blocks
            if (widthBlocks < 2)
            {
                widthBlocks = 2;
            }
            
            if (heightBlocks < 2)
            {
                heightBlocks = 2;
            }
            
            uint32_t dataSize = widthBlocks * heightBlocks * ((blockSize  * bpp) / 8);
            
            [imageData addObject:[NSData dataWithBytes:bytes+dataOffset length:dataSize]];
            
            dataOffset += dataSize;
            
            width = MYMAX(width >> 1, 1);
            height = MYMAX(height >> 1, 1);
        }
        
        // Stores mipmaps.
        width = header->width;
        height = header->height;
        
        auto pixelFormat = MTLPixelFormatPVRTC_RGBA_4BPP;
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:(mipmaps == Mipmaps::Generate ? YES : NO)];
        metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:descriptor];
        metalTexture.label = @"Texture 2D PVR2";

        MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
        
        NSData* data = [imageData objectAtIndex:0];
        [metalTexture replaceRegion:region mipmapLevel:0 withBytes:[data bytes] bytesPerRow:0];
        
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

        [imageData removeAllObjects];
    }
#endif
}

void ae3d::Texture2D::LoadPVRv3( const char* path )
{
#if !TARGET_OS_IPHONE
    System::Assert( false, "PVR3 loading only supported on iOS");
    return;
#else
    NSString* pvrtcNSString = [NSString stringWithUTF8String: path];
    NSData* fileData = [NSData dataWithContentsOfFile:pvrtcNSString];
    PVRv3Header* header = (PVRv3Header*) [fileData bytes];

    const uint64_t rgb2bpp = 0;
    const uint64_t rgba2bpp = 1;
    const uint64_t rgb4bpp = 2;
    const uint64_t rgba4bpp = 3;
    
    width = CFSwapInt32LittleToHost( header->width );
    height = CFSwapInt32LittleToHost( header->height );
    opaque = header->pixelFormat == rgb2bpp || header->pixelFormat == rgb4bpp;

    uint32_t dataLength = (uint32_t)[fileData length] - (sizeof( PVRv3Header ) + CFSwapInt32LittleToHost( header->metadataLength ) );

    // The following line has - 4 because on 64-bit code the header size is 56 bytes due to alignment, but in the file it's only 52 bytes.
    uint8_t* bytes = ((uint8_t *)[fileData bytes]) + sizeof( PVRv3Header ) + CFSwapInt32LittleToHost( header->metadataLength ) - 4;
    
    uint32_t blockWidth = 0, blockHeight = 0, bitsPerPixel = 0;
    uint32_t mipCount = CFSwapInt32LittleToHost( header->mipmapCount );
    const bool isLinear = (CFSwapInt32LittleToHost( header->colorSpace ) == 0);
    
    NSMutableArray *levelDatas = [NSMutableArray arrayWithCapacity:MYMAX(1, mipCount)];
    
    if (header->pixelFormat == rgb4bpp || header->pixelFormat == rgba4bpp)
    {
        blockWidth = blockHeight = 4;
        bitsPerPixel = 4;
    }
    else
    {
        blockWidth = 8;
        blockHeight = 4;
        bitsPerPixel = 2;
    }
    
    uint32_t blockSize = blockWidth * blockHeight;
    uint32_t dataOffset = 0;
    uint32_t levelWidth = width, levelHeight = height;

    while (dataOffset < dataLength)
    {
        uint32_t widthInBlocks = levelWidth / blockWidth;
        uint32_t heightInBlocks = levelHeight / blockHeight;
        
        uint32_t mipSize = widthInBlocks * heightInBlocks * ((blockSize * bitsPerPixel) / 8);
        
        if (mipSize < 32)
        {
            mipSize = 32;
        }
        
        NSData *mipData = [NSData dataWithBytes:bytes + dataOffset length:mipSize];
        [levelDatas addObject:mipData];
        
        dataOffset += mipSize;
        
        levelWidth = MYMAX( levelWidth / 2, 1 );
        levelHeight = MYMAX( levelHeight / 2, 1 );
    }
    
    MTLPixelFormat pixelFormat = MTLPixelFormatPVRTC_RGBA_2BPP;
    
    if (rgb2bpp)
    {
        pixelFormat = isLinear ? MTLPixelFormatPVRTC_RGB_2BPP_sRGB : MTLPixelFormatPVRTC_RGB_2BPP;
    }
    else if (rgba2bpp)
    {
        pixelFormat = isLinear ? MTLPixelFormatPVRTC_RGBA_2BPP_sRGB : MTLPixelFormatPVRTC_RGBA_2BPP;
    }
    else if (rgb4bpp)
    {
        pixelFormat = isLinear ? MTLPixelFormatPVRTC_RGB_4BPP_sRGB : MTLPixelFormatPVRTC_RGB_4BPP;
    }
    else if (rgba4bpp)
    {
        pixelFormat = isLinear ? MTLPixelFormatPVRTC_RGBA_4BPP_sRGB : MTLPixelFormatPVRTC_RGBA_4BPP_sRGB;
    }
    else
    {
        System::Print( "Unknown pixel format in %s\n", path );
        return;
    }

    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                                                          width:width
                                                                                         height:height
                                                                                      mipmapped:(mipmaps == Mipmaps::Generate ? YES : NO)];
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:descriptor];
    metalTexture.label = @"Texture 2D PVR3";

    MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
    
    NSData* data = [levelDatas objectAtIndex:0];
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:[data bytes] bytesPerRow:0];
    
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

    [levelDatas removeAllObjects];
#endif
}
