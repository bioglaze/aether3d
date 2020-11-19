#include "Texture2D.hpp"
#include <string>
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#if TARGET_OS_IPHONE
#define STBI_NEON
#endif
#include "stb_image.c"
#include "DDSLoader.hpp"
#include "FileSystem.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"

#define MYMAX(x, y) ((x) > (y) ? (x) : (y))

extern id <MTLCommandQueue> commandQueue;
bool HasStbExtension( const std::string& path ); // Defined in TextureCommon.cpp
int tex2dMemoryUsage = 0;

namespace MathUtil
{
    int Min( int a, int b );
    int Max( int a, int b );
}

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

struct KTXHeader
{
    uint8_t identifier[ 12 ];
    uint32_t endianness;
    uint32_t glType;
    uint32_t glTypeSize;
    uint32_t glFormat;
    uint32_t glInternalFormat;
    uint32_t glBaseInternalFormat;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t arrayElementCount;
    uint32_t faceCount;
    uint32_t mipmapCount;
    uint32_t keyValueDataLength;
};

namespace
{
    ae3d::Texture2D defaultTexture;
}

#if TARGET_OS_IPHONE
enum KTXInternalFormat
{
    KTXInternalFormatASTC_4x4   = 37808,
    KTXInternalFormatASTC_5x4   = 37809,
    KTXInternalFormatASTC_5x5   = 37810,
    KTXInternalFormatASTC_6x5   = 37811,
    KTXInternalFormatASTC_6x6   = 37812,
    KTXInternalFormatASTC_8x5   = 37813,
    KTXInternalFormatASTC_8x6   = 37814,
    KTXInternalFormatASTC_8x8   = 37815,
    KTXInternalFormatASTC_10x5  = 37816,
    KTXInternalFormatASTC_10x6  = 37817,
    KTXInternalFormatASTC_10x8  = 37818,
    KTXInternalFormatASTC_10x10 = 37819,
    KTXInternalFormatASTC_12x10 = 37820,
    KTXInternalFormatASTC_12x12 = 37821,

    KTXInternalFormatASTC_4x4_sRGB   = 37840,
    KTXInternalFormatASTC_5x4_sRGB   = 37841,
    KTXInternalFormatASTC_5x5_sRGB   = 37842,
    KTXInternalFormatASTC_6x5_sRGB   = 37843,
    KTXInternalFormatASTC_6x6_sRGB   = 37844,
    KTXInternalFormatASTC_8x5_sRGB   = 37845,
    KTXInternalFormatASTC_8x6_sRGB   = 37846,
    KTXInternalFormatASTC_8x8_sRGB   = 37847,
    KTXInternalFormatASTC_10x5_sRGB  = 37848,
    KTXInternalFormatASTC_10x6_sRGB  = 37849,
    KTXInternalFormatASTC_10x8_sRGB  = 37850,
    KTXInternalFormatASTC_10x10_sRGB = 37851,
    KTXInternalFormatASTC_12x10_sRGB = 37852,
    KTXInternalFormatASTC_12x12_sRGB = 37853
};

void GetASTCBlockDimension( MTLPixelFormat pixelFormat, unsigned& outBlockWidth, unsigned& outBlockHeight )
{
    switch (pixelFormat)
    {
        case MTLPixelFormatASTC_4x4_LDR:
        case MTLPixelFormatASTC_4x4_sRGB:
            outBlockHeight = 4;
            outBlockWidth = 4;
            break;
        case MTLPixelFormatASTC_5x4_LDR:
        case MTLPixelFormatASTC_5x4_sRGB:
            outBlockHeight = 5;
            outBlockWidth = 4;
            break;
        case MTLPixelFormatASTC_5x5_LDR:
        case MTLPixelFormatASTC_5x5_sRGB:
            outBlockHeight = 5;
            outBlockWidth = 5;
            break;
        case MTLPixelFormatASTC_6x5_LDR:
        case MTLPixelFormatASTC_6x5_sRGB:
            outBlockHeight = 6;
            outBlockWidth = 5;
            break;
        case MTLPixelFormatASTC_6x6_LDR:
        case MTLPixelFormatASTC_6x6_sRGB:
            outBlockHeight = 6;
            outBlockWidth = 6;
            break;
        case MTLPixelFormatASTC_8x5_LDR:
        case MTLPixelFormatASTC_8x5_sRGB:
            outBlockHeight = 8;
            outBlockWidth = 5;
            break;
        case MTLPixelFormatASTC_8x6_LDR:
        case MTLPixelFormatASTC_8x6_sRGB:
            outBlockHeight = 8;
            outBlockWidth = 6;
            break;
        case MTLPixelFormatASTC_8x8_LDR:
        case MTLPixelFormatASTC_8x8_sRGB:
            outBlockWidth = 8;
            outBlockHeight = 8;
            break;
        case MTLPixelFormatASTC_10x5_LDR:
        case MTLPixelFormatASTC_10x5_sRGB:
            outBlockHeight = 10;
            outBlockWidth = 5;
            break;
        case MTLPixelFormatASTC_10x6_LDR:
        case MTLPixelFormatASTC_10x6_sRGB:
            outBlockHeight = 10;
            outBlockWidth = 6;
            break;
        case MTLPixelFormatASTC_10x8_LDR:
        case MTLPixelFormatASTC_10x8_sRGB:
            outBlockHeight = 10;
            outBlockWidth = 8;
            break;
        case MTLPixelFormatASTC_10x10_LDR:
        case MTLPixelFormatASTC_10x10_sRGB:
            outBlockHeight = 10;
            outBlockWidth = 10;
            break;
        case MTLPixelFormatASTC_12x10_LDR:
        case MTLPixelFormatASTC_12x10_sRGB:
            outBlockHeight = 12;
            outBlockWidth = 10;
            break;
        case MTLPixelFormatASTC_12x12_LDR:
        case MTLPixelFormatASTC_12x12_sRGB:
            outBlockHeight = 12;
            outBlockWidth = 12;
            break;
        default:
            outBlockHeight = 0;
            outBlockWidth = 0;
            break;
    }
}

MTLPixelFormat GetPixelFormatFromKTXFormat( KTXInternalFormat internalFormat )
{
    switch (internalFormat)
    {
        case KTXInternalFormatASTC_4x4: return MTLPixelFormatASTC_4x4_LDR;
        case KTXInternalFormatASTC_5x4: return MTLPixelFormatASTC_5x4_LDR;
        case KTXInternalFormatASTC_5x5: return MTLPixelFormatASTC_5x5_LDR;
        case KTXInternalFormatASTC_6x5: return MTLPixelFormatASTC_6x5_LDR;
        case KTXInternalFormatASTC_6x6: return MTLPixelFormatASTC_6x6_LDR;
        case KTXInternalFormatASTC_8x5: return MTLPixelFormatASTC_8x5_LDR;
        case KTXInternalFormatASTC_8x6: return MTLPixelFormatASTC_8x6_LDR;
        case KTXInternalFormatASTC_8x8: return MTLPixelFormatASTC_8x8_LDR;
        case KTXInternalFormatASTC_10x5: return MTLPixelFormatASTC_10x5_LDR;
        case KTXInternalFormatASTC_10x6: return MTLPixelFormatASTC_10x6_LDR;
        case KTXInternalFormatASTC_10x8: return MTLPixelFormatASTC_10x8_LDR;
        case KTXInternalFormatASTC_10x10: return MTLPixelFormatASTC_10x10_LDR;
        case KTXInternalFormatASTC_12x10: return MTLPixelFormatASTC_12x10_LDR;
        case KTXInternalFormatASTC_12x12: return MTLPixelFormatASTC_12x12_LDR;
        case KTXInternalFormatASTC_4x4_sRGB: return MTLPixelFormatASTC_4x4_sRGB;
        case KTXInternalFormatASTC_5x4_sRGB: return MTLPixelFormatASTC_5x4_sRGB;
        case KTXInternalFormatASTC_5x5_sRGB: return MTLPixelFormatASTC_5x5_sRGB;
        case KTXInternalFormatASTC_6x5_sRGB: return MTLPixelFormatASTC_6x5_sRGB;
        case KTXInternalFormatASTC_6x6_sRGB: return MTLPixelFormatASTC_6x6_sRGB;
        case KTXInternalFormatASTC_8x5_sRGB: return MTLPixelFormatASTC_8x5_sRGB;
        case KTXInternalFormatASTC_8x6_sRGB: return MTLPixelFormatASTC_8x6_sRGB;
        case KTXInternalFormatASTC_8x8_sRGB: return MTLPixelFormatASTC_8x8_sRGB;
        case KTXInternalFormatASTC_10x5_sRGB: return MTLPixelFormatASTC_10x5_sRGB;
        case KTXInternalFormatASTC_10x6_sRGB: return MTLPixelFormatASTC_10x6_sRGB;
        case KTXInternalFormatASTC_10x8_sRGB: return MTLPixelFormatASTC_10x8_sRGB;
        case KTXInternalFormatASTC_10x10_sRGB: return MTLPixelFormatASTC_10x10_sRGB;
        case KTXInternalFormatASTC_12x10_sRGB: return MTLPixelFormatASTC_12x10_sRGB;
        case KTXInternalFormatASTC_12x12_sRGB: return MTLPixelFormatASTC_12x12_sRGB;
        default:
            return MTLPixelFormatInvalid;
    }
}
#endif

ae3d::Texture2D* ae3d::Texture2D::GetDefaultTexture()
{
    if (defaultTexture.GetWidth() == 0 && GfxDevice::GetMetalDevice() != nullptr)
    {
        defaultTexture.width = 128;
        defaultTexture.height = 128;
        defaultTexture.opaque = false;
        
        MTLTextureDescriptor* textureDescriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
                                                           width:defaultTexture.width
                                                          height:defaultTexture.height
                                                       mipmapped:NO];
        id<MTLTexture> stagingTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];
        stagingTexture.label = @"default texture2d";
        
        MTLTextureDescriptor* textureDescriptor2 =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm_sRGB
                                                           width:defaultTexture.width
                                                          height:defaultTexture.height
                                                       mipmapped:NO];
        textureDescriptor2.usage = MTLTextureUsageShaderRead;
        textureDescriptor2.storageMode = MTLStorageModePrivate;
        defaultTexture.metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor2];
        defaultTexture.metalTexture.label = stagingTexture.label;
        
        const int components = 4;
        int data[ 128 * 128 * components ] = { 0xFFC0CB };
        
        MTLRegion region = MTLRegionMake2D(0, 0, defaultTexture.width, defaultTexture.height);
        const int bytesPerRow = components * defaultTexture.width;
        [stagingTexture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerRow];

        id <MTLCommandBuffer> cmd_buffer =     [commandQueue commandBuffer];
        cmd_buffer.label = @"BlitCommandBuffer";
        id <MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
        [blit_encoder copyFromTexture:stagingTexture
                          sourceSlice:0
                          sourceLevel:0
                         sourceOrigin:MTLOriginMake( 0, 0, 0 )
                           sourceSize:MTLSizeMake( defaultTexture.width, defaultTexture.height, 1 )
                            toTexture:defaultTexture.metalTexture
                     destinationSlice:0
                     destinationLevel:0
                    destinationOrigin:MTLOriginMake( 0, 0, 0 ) ];
        [blit_encoder endEncoding];
        [cmd_buffer commit];
        [cmd_buffer waitUntilCompleted];
    }

    return &defaultTexture;
}

void ae3d::Texture2D::CreateUAV( int aWidth, int aHeight, const char* debugName, DataType format )
{
    width = aWidth;
    height = aHeight;
    wrap = TextureWrap::Repeat;
    filter = TextureFilter::Linear;
    opaque = true;

    MTLPixelFormat pixelFormat = MTLPixelFormatRGBA8Unorm_sRGB;
    
    if (format == DataType::Float)
    {
        pixelFormat = MTLPixelFormatRGBA32Float;
    }
    else if (format == DataType::Float16)
    {
        pixelFormat = MTLPixelFormatRGBA16Float;
    }
    else if (format == DataType::R32G32)
    {
        pixelFormat = MTLPixelFormatRG32Float;
    }
    else if (format == DataType::R32F)
    {
        pixelFormat = MTLPixelFormatR32Float;
    }

    MTLTextureDescriptor* textureDescriptor =
    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                       width:width
                                                      height:height
                                                   mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
    textureDescriptor.storageMode = MTLStorageModePrivate;
    
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];
    metalTexture.label = [NSString stringWithUTF8String:debugName];
    tex2dMemoryUsage += [metalTexture allocatedSize];
}

void ae3d::Texture2D::LoadFromData( const void* imageData, int aWidth, int aHeight, const char* debugName, DataType format )
{
    width = aWidth;
    height = aHeight;
    wrap = TextureWrap::Repeat;
    filter = TextureFilter::Linear;
    opaque = true;
    int bytesPerRow = width * 4;

    MTLPixelFormat pixelFormat = colorSpace == ColorSpace::Linear ? MTLPixelFormatRGBA8Unorm : MTLPixelFormatRGBA8Unorm_sRGB;
    
    if (format == DataType::Float)
    {
        pixelFormat = MTLPixelFormatRGBA32Float;
        bytesPerRow = width * 4 * sizeof( float );
    }
    else if (format == DataType::Float16)
    {
        pixelFormat = MTLPixelFormatRGBA16Float;
        bytesPerRow = width * 4 * sizeof( float ) / 2;
    }
    else if (format == DataType::R32G32)
    {
        pixelFormat = MTLPixelFormatRG32Float;
        bytesPerRow = width * 2 * sizeof( float );
    }
    else if (format == DataType::R32F)
    {
        pixelFormat = MTLPixelFormatR32Float;
        bytesPerRow = width * 1 * sizeof( float );
    }

    NSString* debugNameStr = [NSString stringWithUTF8String:debugName ];
    
    MTLTextureDescriptor* textureDescriptor =
    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                       width:width
                                                      height:height
                                                   mipmapped:(mipmaps == Mipmaps::None ? NO : YES)];
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    id<MTLTexture> stagingTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];
    stagingTexture.label = debugNameStr;
        
    MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
    [stagingTexture replaceRegion:region mipmapLevel:0 withBytes:imageData bytesPerRow:bytesPerRow];

    MTLTextureDescriptor* textureDescriptor2 =
    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                       width:width
                                                      height:height
                                                   mipmapped:(mipmaps == Mipmaps::None ? NO : YES)];
    textureDescriptor2.usage = MTLTextureUsageShaderRead;
    textureDescriptor2.storageMode = MTLStorageModePrivate;
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor2];
    metalTexture.label = stagingTexture.label;
    tex2dMemoryUsage += [metalTexture allocatedSize];
    
    id <MTLCommandBuffer> cmd_buffer =     [commandQueue commandBuffer];
    cmd_buffer.label = @"BlitCommandBuffer";
    id <MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
    [blit_encoder copyFromTexture:stagingTexture
                      sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake( 0, 0, 0 )
                       sourceSize:MTLSizeMake( width, height, 1 )
                        toTexture:metalTexture
                 destinationSlice:0
                 destinationLevel:0
                destinationOrigin:MTLOriginMake( 0, 0, 0 ) ];
    [blit_encoder endEncoding];
    [cmd_buffer commit];
    [cmd_buffer waitUntilCompleted];

    if (mipmaps == Mipmaps::Generate)
    {
        id<MTLCommandQueue> commandQueue2 = [GfxDevice::GetMetalDevice() newCommandQueue];
        id<MTLCommandBuffer> commandBuffer = [commandQueue2 commandBuffer];
        id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
        [commandEncoder generateMipmapsForTexture:metalTexture];
        [commandEncoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }
}

void ae3d::Texture2D::SetLayout( TextureLayout aLayout )
{
    // Not needed on Metal.
}

void ae3d::Texture2D::Load( const FileSystem::FileContentsData& fileContents, TextureWrap aWrap, TextureFilter aFilter, Mipmaps aMipmaps, ColorSpace aColorSpace, Anisotropy aAnisotropy )
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
    
    const bool isPVR = fileContents.path.find( ".pvr" ) != std::string::npos || fileContents.path.find( ".PVR" ) != std::string::npos;
    const bool isDDS = fileContents.path.find( ".dds" ) != std::string::npos || fileContents.path.find( ".DDS" ) != std::string::npos;
    const bool isASTC = fileContents.path.find( ".ktx" ) != std::string::npos || fileContents.path.find( ".ktx" ) != std::string::npos;

    if (HasStbExtension( fileContents.path ))
    {
        LoadSTB( fileContents );
    }
    else if (isPVR)
    {
        NSString* pvrtcNSString = [NSString stringWithUTF8String: fileContents.path.c_str()];
        NSData* fileData = [NSData dataWithContentsOfFile:pvrtcNSString];
        PVRv2Header* header = (PVRv2Header*) [fileData bytes];
        uint32_t version = CFSwapInt32LittleToHost( header->headerLength );

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
        DDSLoader::LoadResult ddsLoadResult = DDSLoader::Load( fileContents, width, height, opaque, output );
        
        if (ddsLoadResult != DDSLoader::LoadResult::Success)
        {
            ae3d::System::Print( "Could not load %s\n", fileContents.path.c_str() );
            return;
        }
        
        int multiplier = 2;
        
        MTLPixelFormat pixelFormat = MTLPixelFormatRGBA8Unorm;
        
        if (output.format == DDSLoader::Format::BC1)
        {
            pixelFormat = colorSpace == ColorSpace::Linear ? MTLPixelFormatBC1_RGBA : MTLPixelFormatBC1_RGBA_sRGB;
        }
        else if (output.format == DDSLoader::Format::BC2)
        {
            pixelFormat = colorSpace == ColorSpace::Linear ? MTLPixelFormatBC2_RGBA : MTLPixelFormatBC2_RGBA_sRGB;
            multiplier = 4;
        }
        else if (output.format == DDSLoader::Format::BC3)
        {
            pixelFormat = colorSpace == ColorSpace::Linear ? MTLPixelFormatBC3_RGBA : MTLPixelFormatBC3_RGBA_sRGB;
            multiplier = 4;
        }
        else if (output.format == DDSLoader::Format::BC4U)
        {
            pixelFormat = MTLPixelFormatBC4_RUnorm;
            multiplier = 2;
        }
        else if (output.format == DDSLoader::Format::BC4S)
        {
            pixelFormat = MTLPixelFormatBC4_RSnorm;
            multiplier = 2;
        }
        else if (output.format == DDSLoader::Format::BC5U)
        {
            pixelFormat = MTLPixelFormatBC5_RGUnorm;
            multiplier = 4;
        }
        else if (output.format == DDSLoader::Format::BC5S)
        {
            pixelFormat = MTLPixelFormatBC5_RGSnorm;
            multiplier = 4;
        }

        MTLTextureDescriptor* textureDescriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                           width:width
                                                          height:height
                                                       mipmapped:(mipmaps == Mipmaps::None ? NO : YES)];
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        id< MTLTexture > stagingTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];
        
        const std::size_t pos = fileContents.path.find_last_of( "/" );
        if (pos != std::string::npos)
        {
            std::string fileName = fileContents.path.substr( pos );
            stagingTexture.label = [NSString stringWithUTF8String:fileName.c_str()];
        }
        else
        {
            stagingTexture.label = [NSString stringWithUTF8String:fileContents.path.c_str()];
        }

        mipLevelCount = mipmaps == Mipmaps::Generate ? output.dataOffsets.count : 1;

        for (int mipIndex = 0; mipIndex < mipLevelCount; ++mipIndex)
        {
            const int mipWidth = MathUtil::Max( width >> mipIndex, 1 );
            const int mipHeight = MathUtil::Max( height >> mipIndex, 1 );
            const int mipBytesPerRow = mipWidth <= 2 ? 8 : mipWidth * multiplier;
            
            MTLRegion region = MTLRegionMake2D( 0, 0, mipWidth, mipHeight );
            [stagingTexture replaceRegion:region mipmapLevel:mipIndex withBytes:&output.imageData[ output.dataOffsets[ mipIndex ] ] bytesPerRow:mipBytesPerRow];
        }
        
        MTLTextureDescriptor* textureDescriptor2 =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                           width:width
                                                          height:height
                                                       mipmapped:(mipmaps == Mipmaps::None ? NO : YES)];
        textureDescriptor2.usage = MTLTextureUsageShaderRead;
        textureDescriptor2.storageMode = MTLStorageModePrivate;
        metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor2];
        metalTexture.label = stagingTexture.label;
        
        for (int mipIndex = 0; mipIndex < mipLevelCount; ++mipIndex)
        {
            const int mipWidth = MathUtil::Max( width >> mipIndex, 1 );
            const int mipHeight = MathUtil::Max( height >> mipIndex, 1 );

            id <MTLCommandBuffer> cmd_buffer =     [commandQueue commandBuffer];
            cmd_buffer.label = @"BlitCommandBuffer";
            id <MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
            [blit_encoder copyFromTexture:stagingTexture
                              sourceSlice:0
                              sourceLevel:mipIndex
                             sourceOrigin:MTLOriginMake( 0, 0, 0 )
                               sourceSize:MTLSizeMake( mipWidth, mipHeight, 1 )
                                toTexture:metalTexture
                         destinationSlice:0
                         destinationLevel:mipIndex
                        destinationOrigin:MTLOriginMake( 0, 0, 0 ) ];
            [blit_encoder endEncoding];
            [cmd_buffer commit];
            [cmd_buffer waitUntilCompleted];
        }
#else
        ae3d::System::Print( ".dds loading not supported on iOS. Tried to load %s\n", fileContents.path.c_str() );
#endif
    }
    else if (isASTC)
    {
#if TARGET_OS_IPHONE
        NSString* astcNSString = [NSString stringWithUTF8String: fileContents.path.c_str()];
        NSData* fileData = [NSData dataWithContentsOfFile:astcNSString];
        KTXHeader* ktxHeader = (KTXHeader*) [fileData bytes];

        char* format = (char *)(ktxHeader->identifier + 1);
        const bool isValid = strncmp( format, "KTX 11", 6 ) == 0;

        if (!isValid)
        {
            ae3d::System::Print( "%s doesn't have a valid KTX header!\n", fileContents.path.c_str() );
        }
        
        width = ktxHeader->width;
        height = ktxHeader->height;
        mipLevelCount = 1;
        MTLPixelFormat pixelFormat = GetPixelFormatFromKTXFormat( (KTXInternalFormat)ktxHeader->glInternalFormat );
        
        if (pixelFormat == MTLPixelFormatInvalid)
        {
            ae3d::System::Print( "%s doesn't contain ASTC compressed pixel data!\n", fileContents.path.c_str() );
        }
        
        unsigned blockWidth, blockHeight;
        GetASTCBlockDimension( pixelFormat, blockWidth, blockHeight );
        const unsigned blockSize = 16;
        int multiplier = (blockSize / blockWidth);

        mipLevelCount = 1;
        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:NO];
        metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:descriptor];

        const std::size_t pos = fileContents.path.find_last_of( "/" );
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
        unsigned offset = sizeof( KTXHeader ) + ktxHeader->keyValueDataLength + 4;

        [metalTexture replaceRegion:region mipmapLevel:0 withBytes:&fileContents.data[ offset ] bytesPerRow:width * multiplier];
#else
     ae3d::System::Print( ".astc loading not supported on macOS. Tried to load %s\n", fileContents.path.c_str() );
#endif
    }
    else
    {
        ae3d::System::Print( "Unhandled texture extension in %s\n", fileContents.path.c_str() );
    }
    
    tex2dMemoryUsage += [metalTexture allocatedSize];
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
    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:colorSpace == ColorSpace::Linear ? MTLPixelFormatRGBA8Unorm : MTLPixelFormatRGBA8Unorm_sRGB
                                                       width:width
                                                      height:height
                                                   mipmapped:(mipmaps == Mipmaps::None ? NO : YES)];
    id<MTLTexture> stagingTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];
    
    std::size_t pos = fileContents.path.find_last_of( "/" );
    if (pos != std::string::npos)
    {
        std::string fileName = fileContents.path.substr( pos );
        stagingTexture.label = [NSString stringWithUTF8String:fileName.c_str()];
    }
    else
    {
        stagingTexture.label = [NSString stringWithUTF8String:fileContents.path.c_str()];
    }
    
    const int bytesPerRow = width * 4;

    MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
    [stagingTexture replaceRegion:region mipmapLevel:0 withBytes:data bytesPerRow:bytesPerRow];
    
    MTLTextureDescriptor* textureDescriptor2 =
    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:colorSpace == ColorSpace::Linear ? MTLPixelFormatRGBA8Unorm : MTLPixelFormatRGBA8Unorm_sRGB
                                                       width:width
                                                      height:height
                                                   mipmapped:(mipmaps == Mipmaps::None ? NO : YES)];
    textureDescriptor2.usage = MTLTextureUsageShaderRead;
    textureDescriptor2.storageMode = MTLStorageModePrivate;
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor2];
    metalTexture.label = stagingTexture.label;
    
    id <MTLCommandBuffer> cmd_buffer =     [commandQueue commandBuffer];
    cmd_buffer.label = @"BlitCommandBuffer";
    id <MTLBlitCommandEncoder> blit_encoder = [cmd_buffer blitCommandEncoder];
    [blit_encoder copyFromTexture:stagingTexture
                    sourceSlice:0
                      sourceLevel:0
                     sourceOrigin:MTLOriginMake( 0, 0, 0 )
                       sourceSize:MTLSizeMake( width, height, 1 )
                        toTexture:metalTexture
               destinationSlice:0
                 destinationLevel:0
                destinationOrigin:MTLOriginMake( 0, 0, 0 ) ];
    [blit_encoder endEncoding];
    [cmd_buffer commit];
    [cmd_buffer waitUntilCompleted];

    if (mipmaps == Mipmaps::Generate)
    {
        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
        [commandEncoder generateMipmapsForTexture:metalTexture];
        [commandEncoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    stbi_image_free( data );
}

void ae3d::Texture2D::LoadPVRv2( const char* path )
{
#if !TARGET_OS_IPHONE
    System::Print( "PVR2 loading only supported on iOS. Tried to load %s\n", path);
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
        metalTexture.label = [NSString stringWithFormat:@"%s", path];
        
        MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
        
        NSData* data = [imageData objectAtIndex:0];
        [metalTexture replaceRegion:region mipmapLevel:0 withBytes:[data bytes] bytesPerRow:0];
        
        if (mipmaps == Mipmaps::Generate)
        {
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
            [commandEncoder generateMipmapsForTexture:metalTexture];
            [commandEncoder endEncoding];
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
        }

        [imageData removeAllObjects];
    }
#endif
}

void ae3d::Texture2D::LoadPVRv3( const char* path )
{
#if !TARGET_OS_IPHONE
    System::Print( "PVR3 loading only supported on iOS. Tried to load %s\n", path);
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
    metalTexture.label = [NSString stringWithFormat:@"%s", path];
    
    MTLRegion region = MTLRegionMake2D( 0, 0, width, height );
    
    NSData* data = [levelDatas objectAtIndex:0];
    [metalTexture replaceRegion:region mipmapLevel:0 withBytes:[data bytes] bytesPerRow:0];
    
    if (mipmaps == Mipmaps::Generate)
    {
        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
        [commandEncoder generateMipmapsForTexture:metalTexture];
        [commandEncoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    [levelDatas removeAllObjects];
#endif
}
