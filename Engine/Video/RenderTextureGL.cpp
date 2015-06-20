#include <GL/glxw.h>
#include "RenderTexture.hpp"
#include "GfxDevice.hpp"
#include "System.hpp"

void ae3d::RenderTexture2D::Create( int aWidth, int aHeight, TextureWrap aWrap, TextureFilter aFilter )
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
    
    handle = GfxDevice::CreateTextureId();
    glBindTexture( GL_TEXTURE_2D, handle );

    if (GfxDevice::HasExtension( "KHR_debug" ))
    {
        glObjectLabel( GL_TEXTURE, handle, 17, "render_texture_2d" );
    }

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter == TextureFilter::Nearest ? GL_NEAREST : (mipmaps == Mipmaps::Generate ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR ) );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap == TextureWrap::Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap == TextureWrap::Repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

    glBindTexture( GL_TEXTURE_2D, 0 );
    
    // Creates a renderbuffer object to store depth info.
    rboId = GfxDevice::CreateRboId();
    glBindRenderbuffer( GL_RENDERBUFFER, rboId );
    glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height );
    glBindRenderbuffer( GL_RENDERBUFFER, 0 );
    
    // Creates the Frame Buffer Object.
    fboId = GfxDevice::CreateFboId();
    //glBindFramebuffer( GL_FRAMEBUFFER, fboId );
    GfxDevice::SetRenderTarget( this );
    
    GfxDevice::ErrorCheck( "CreateRenderTexture2D middle" );
    
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, handle, 0 );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboId );
    //RendererImpl::Instance().ClearScreen();
    
    GfxDevice::ErrorCheckFBO();
    GfxDevice::ErrorCheck( "CreateRenderTexture2D end" );
}
