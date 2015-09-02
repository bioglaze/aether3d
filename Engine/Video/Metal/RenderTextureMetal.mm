#include "RenderTexture.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"

void ae3d::RenderTexture::Create2D( int aWidth, int aHeight, TextureWrap aWrap, TextureFilter aFilter )
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
}

void ae3d::RenderTexture::CreateCube( int aDimension, TextureWrap aWrap, TextureFilter aFilter )
{
    System::Print( "Cube map RT not implemented!\n" );
}
