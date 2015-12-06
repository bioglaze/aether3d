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
    
    MTLTextureDescriptor* textureDescriptor =
    [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                       width:width
                                                      height:height
                                                   mipmapped:NO];
    metalTexture = [GfxDevice::GetMetalDevice() newTextureWithDescriptor:textureDescriptor];

    if (metalTexture == nullptr)
    {
        System::Print("Failed to create a render texture!\n");
    }
    
    metalTexture.label = @"Render Texture";
}

void ae3d::RenderTexture::CreateCube( int aDimension, DataType dataType, TextureWrap aWrap, TextureFilter aFilter )
{
    System::Print( "Cube map RT not implemented!\n" );
}
