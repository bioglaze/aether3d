#include "GfxDevice.hpp"
#include <GL/glxw.h>

void ae3d::GfxDevice::ClearScreen(unsigned clearFlags)
{
    GLbitfield mask = 0;

    if ((clearFlags & ClearFlags::Color) != 0)
    {
        mask |= GL_COLOR_BUFFER_BIT;
    }
    if ((clearFlags & ClearFlags::Depth) != 0)
    {
        mask |= GL_DEPTH_BUFFER_BIT;
    }

    glClear( mask );
}

void ae3d::GfxDevice::SetClearColor(float red, float green, float blue)
{
    glClearColor( red, green, blue, 1 );
}

