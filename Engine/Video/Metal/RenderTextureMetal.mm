#include "RenderTexture.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"

void ae3d::RenderTexture::Create2D( int aWidth, int aHeight, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter )
{
    if (aWidth <= 0 || aHeight <= 0)
    {
        System::Print( "Render texture has invalid dimension!\n" );
        return;
    }
    
    width = aWidth;
    height = aHeight;
    wrap = aWrap;
    filter = aFilter;
    handle = 1;
    isRenderTexture = true;
    dataType = aDataType;
    
    MTLTextureDescriptor* textureDescriptor =
    //[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:(dataType == DataType::UByte ? MTLPixelFormatBGRA8Unorm : MTLPixelFormatRGBA32Float)
                                                       width:width
                                                      height:height
                                                   mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];

    if (metalTexture == nullptr)
    {
        System::Print( "Failed to create a render texture 2D!\n" );
    }
    
    metalTexture.label = @"Render Texture 2D";
}

void ae3d::RenderTexture::CreateCube( int aDimension, DataType aDataType, TextureWrap aWrap, TextureFilter aFilter )
{
    isCube = true;
    width = aDimension;
    height = aDimension;
    wrap = aWrap;
    filter = aFilter;
    handle = 1;
    isRenderTexture = true;
    dataType = aDataType;
    
    MTLTextureDescriptor* textureDescriptor =
    [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm//(dataType == DataType::UByte ? MTLPixelFormatBGRA8Unorm : MTLPixelFormatRGBA32Float)
                                                       size:width
                                                   mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];
    
    if (metalTexture == nullptr)
    {
        System::Print( "Failed to create a render texture Cube!\n" );
    }
    
    metalTexture.label = @"Render Texture Cube";

}
