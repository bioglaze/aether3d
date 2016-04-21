#include "RenderTexture.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"

void ae3d::RenderTexture::Create2D( int aWidth, int aHeight, DataType dataType, TextureWrap aWrap, TextureFilter aFilter )
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
    
    MTLTextureDescriptor* textureDescriptor =
    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
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

void ae3d::RenderTexture::CreateCube( int aDimension, DataType dataType, TextureWrap aWrap, TextureFilter aFilter )
{
    isCube = true;
    width = aDimension;
    height = aDimension;
    wrap = aWrap;
    filter = aFilter;
    handle = 1;
    isRenderTexture = true;
    
    MTLTextureDescriptor* textureDescriptor =
    [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
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
