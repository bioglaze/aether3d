#include "GfxDevice.hpp"
#include <vector>
#include <GL/glxw.h>

namespace GfxDeviceGlobal
{
    std::vector< GLuint > vaoIds;
    std::vector< GLuint > vboIds;
    std::vector< GLuint > iboIds;
    std::vector< GLuint > textureIds;
}

void ae3d::GfxDevice::ReleaseGPUObjects()
{
    glDeleteVertexArrays(static_cast<GLsizei>(GfxDeviceGlobal::vaoIds.size()), GfxDeviceGlobal::vaoIds.data());
    glDeleteBuffers(static_cast<GLsizei>(GfxDeviceGlobal::vboIds.size()), GfxDeviceGlobal::vboIds.data());
    glDeleteBuffers(static_cast<GLsizei>(GfxDeviceGlobal::iboIds.size()), GfxDeviceGlobal::iboIds.data());
    glDeleteTextures(static_cast<GLsizei>(GfxDeviceGlobal::textureIds.size()), GfxDeviceGlobal::textureIds.data());
}

unsigned ae3d::GfxDevice::CreateTextureId()
{
    GLuint outId;
    glGenTextures(1, &outId);
    return outId;
}

unsigned ae3d::GfxDevice::CreateVaoId()
{
    GLuint outId;
    glGenVertexArrays(1, &outId);
    return outId;
}

unsigned ae3d::GfxDevice::CreateIboId()
{
    GLuint outId;
    glGenBuffers(1, &outId);
    return outId;
}

unsigned ae3d::GfxDevice::CreateVboId()
{
    GLuint outId;
    glGenBuffers(1, &outId);
    return outId;
}

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

